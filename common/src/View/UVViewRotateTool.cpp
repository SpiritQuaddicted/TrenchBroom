/*
 Copyright (C) 2010-2014 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "UVViewRotateTool.h"

#include "PreferenceManager.h"
#include "Preferences.h"
#include "Model/BrushFace.h"
#include "Model/BrushEdge.h"
#include "Model/BrushVertex.h"
#include "Model/TexCoordSystemHelper.h"
#include "Renderer/Circle.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Ring.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/Transformation.h"
#include "Renderer/Vbo.h"
#include "Renderer/VertexArray.h"
#include "View/ControllerFacade.h"
#include "View/InputState.h"
#include "View/UVViewHelper.h"

namespace TrenchBroom {
    namespace View {
        const Hit::HitType UVViewRotateTool::AngleHandleHit = Hit::freeHitType();
        const float UVViewRotateTool::CenterHandleRadius =  2.0f;
        const float UVViewRotateTool::RotateHandleRadius = 32.0f;
        const float UVViewRotateTool::RotateHandleWidth  =  5.0f;

        UVViewRotateTool::UVViewRotateTool(MapDocumentWPtr document, ControllerWPtr controller, UVViewHelper& helper) :
        ToolImpl(document, controller),
        m_helper(helper) {}
        
        void UVViewRotateTool::doPick(const InputState& inputState, Hits& hits) {
            if (!m_helper.valid())
                return;

            const Model::BrushFace* face = m_helper.face();
            const Mat4x4 fromFace = face->fromTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true);

            const Plane3& boundary = face->boundary();
            const Mat4x4 toPlane   = planeProjectionMatrix4(boundary.distance, boundary.normal);

            const Ray3& pickRay = inputState.pickRay();
            const FloatType distance = pickRay.intersectWithPlane(boundary.normal, boundary.anchor());
            assert(!Math::isnan(distance));
            const Vec3 hitPoint = pickRay.pointAtDistance(distance);
            
            const Vec3 originOnPlane   = toPlane * fromFace * Vec3(m_helper.originInFaceCoords());
            const Vec3 hitPointOnPlane = toPlane * hitPoint;

            const float zoom = m_helper.cameraZoom();
            const FloatType error = std::abs(RotateHandleRadius / zoom - hitPointOnPlane.distanceTo(originOnPlane));
            if (error <= RotateHandleWidth / zoom)
                hits.addHit(Hit(AngleHandleHit, distance, hitPoint, 0, error));
        }
        
        bool UVViewRotateTool::doStartMouseDrag(const InputState& inputState) {
            assert(m_helper.valid());
            
            if (!inputState.modifierKeysPressed(ModifierKeys::MKNone) ||
                !inputState.mouseButtonsPressed(MouseButtons::MBLeft))
                return false;

            const Hits& hits = inputState.hits();
            const Hit& hit = hits.findFirst(AngleHandleHit, true);

            if (!hit.isMatch())
                return false;

            m_initialPoint = hit.hitPoint();
            m_totalAngleDelta = 0.0f;

            controller()->beginUndoableGroup("Rotate Texture");
            
            return true;
        }
        
        bool UVViewRotateTool::doMouseDrag(const InputState& inputState) {
            assert(m_helper.valid());
            
            Model::BrushFace* face = m_helper.face();
            const Plane3& boundary = face->boundary();
            const Ray3& pickRay    = inputState.pickRay();
            
            const FloatType distance = pickRay.intersectWithPlane(boundary.normal, boundary.anchor());
            const Vec3 point         = pickRay.pointAtDistance(distance);
            
            const Vec3 origin   = face->fromTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true) * Vec3(m_helper.originInFaceCoords());
            const Vec3 axis     = normalize(m_initialPoint - origin);
            const Vec3 vec      = normalize(point - origin);
            
            // invert the measured angle here because texture rotation is clockwise
            const float angle   = static_cast<float>(-Math::degrees(angleBetween(vec, axis, boundary.normal)));
            const float delta   = angle - m_totalAngleDelta;
            const float snapped = Math::correct(snapAngle(face->rotation() + delta) - face->rotation(), 4, 0.0f);
            
            if (snapped != 0.0f) {
                const Model::BrushFaceList applyTo(1, face);
                controller()->rotateTextures(applyTo, snapped, origin);
                m_helper.setOrigin(Vec2f(face->toTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true) * origin));
                m_totalAngleDelta += snapped;
            }
            
            return true;
        }
        
        float UVViewRotateTool::measureAngle(const Vec2f& point) const {
            const Model::BrushFace* face = m_helper.face();
            const Vec2f origin = m_helper.originInFaceCoords();
            return Math::mod(face->measureTextureAngle(origin, point), 360.0f);
        }
        
        float UVViewRotateTool::snapAngle(const float angle) const {
            const Model::BrushFace* face = m_helper.face();
            
            const float angles[] = {
                Math::mod(angle +   0.0f, 360.0f),
                Math::mod(angle +  90.0f, 360.0f),
                Math::mod(angle + 180.0f, 360.0f),
                Math::mod(angle + 270.0f, 360.0f),
            };
            float minDelta = std::numeric_limits<float>::max();
            
            const Model::TexCoordSystemHelper faceCoordSystem = Model::TexCoordSystemHelper::faceCoordSystem(face);
            const Model::BrushEdgeList& edges = face->edges();
            Model::BrushEdgeList::const_iterator it, end;
            for (it = edges.begin(), end = edges.end(); it != end; ++it) {
                const Model::BrushEdge* edge = *it;
                
                const Vec3 startInFaceCoords = faceCoordSystem.worldToTex(edge->start->position);
                const Vec3 endInFaceCoords   = faceCoordSystem.worldToTex(edge->end->position);
                const float edgeAngle        = Math::mod(face->measureTextureAngle(startInFaceCoords, endInFaceCoords), 360.0f);
                
                for (size_t i = 0; i < 4; ++i) {
                    if (std::abs(angles[i] - edgeAngle) < std::abs(minDelta))
                        minDelta = angles[i] - edgeAngle;
                }
            }
            
            if (std::abs(minDelta) < 3.0f)
                return angle - minDelta;
            return angle;
        }

        void UVViewRotateTool::doEndMouseDrag(const InputState& inputState) {
            controller()->closeGroup();
        }
        
        void UVViewRotateTool::doCancelMouseDrag(const InputState& inputState) {
            controller()->rollbackGroup();
        }

        void UVViewRotateTool::doRender(const InputState& inputState, Renderer::RenderContext& renderContext) {
            if (!m_helper.valid())
                return;
            
            const Hits& hits = inputState.hits();
            const Hit& angleHandleHit = hits.findFirst(AngleHandleHit, true);
            
            const bool highlightAngleHandle = angleHandleHit.isMatch() || dragging();
            
            const PreferenceManager& prefs = PreferenceManager::instance();
            const Color& handleColor = prefs.get(Preferences::HandleColor);
            const Color& highlightColor = prefs.get(Preferences::SelectedHandleColor);
            const float cameraZoom = m_helper.cameraZoom();

            const Model::BrushFace* face = m_helper.face();
            const Mat4x4 fromFace = face->fromTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, false);

            const Plane3& boundary = face->boundary();
            const Mat4x4 toPlane = planeProjectionMatrix4(boundary.distance, boundary.normal);
            const Mat4x4 fromPlane = invertedMatrix(toPlane);
            
            const Vec2f originPosition(toPlane * fromFace * Vec3(m_helper.originInFaceCoords()));
            const Vec2f faceCenterPosition(toPlane * m_helper.face()->boundsCenter());

            Renderer::Vbo vbo(0xFFF);
            Renderer::SetVboState vboState(vbo);
            Renderer::Circle center(CenterHandleRadius / cameraZoom, 10, true);
            Renderer::Circle outer(RotateHandleRadius / cameraZoom, 32, false);

            vboState.mapped();
            center.prepare(vbo);
            outer.prepare(vbo);
            vboState.active();

            Renderer::ActiveShader shader(renderContext.shaderManager(), Renderer::Shaders::VaryingPUniformCShader);
            const Renderer::MultiplyModelMatrix toWorldTransform(renderContext.transformation(), fromPlane);
            {
                const Mat4x4 translation = translationMatrix(Vec3(originPosition));
                const Renderer::MultiplyModelMatrix centerTransform(renderContext.transformation(), translation);
                if (highlightAngleHandle)
                    shader.set("Color", highlightColor);
                else
                    shader.set("Color", handleColor);
                outer.render();
            }
            
            {
                const Mat4x4 translation = translationMatrix(Vec3(faceCenterPosition));
                const Renderer::MultiplyModelMatrix centerTransform(renderContext.transformation(), translation);
                shader.set("Color", highlightColor);
                center.render();
            }
        }
    }
}
