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

#include "ParaxialTexCoordSystem.h"

#include "Assets/Texture.h"
#include "Model/BrushFace.h"

namespace TrenchBroom {
    namespace Model {
        const Vec3 ParaxialTexCoordSystem::BaseAxes[] = {
            Vec3( 0.0,  0.0,  1.0), Vec3( 1.0,  0.0,  0.0), Vec3( 0.0, -1.0,  0.0),
            Vec3( 0.0,  0.0, -1.0), Vec3( 1.0,  0.0,  0.0), Vec3( 0.0, -1.0,  0.0),
            Vec3( 1.0,  0.0,  0.0), Vec3( 0.0,  1.0,  0.0), Vec3( 0.0,  0.0, -1.0),
            Vec3(-1.0,  0.0,  0.0), Vec3( 0.0,  1.0,  0.0), Vec3( 0.0,  0.0, -1.0),
            Vec3( 0.0,  1.0,  0.0), Vec3( 1.0,  0.0,  0.0), Vec3( 0.0,  0.0, -1.0),
            Vec3( 0.0, -1.0,  0.0), Vec3( 1.0,  0.0,  0.0), Vec3( 0.0,  0.0, -1.0),
        };
        
        ParaxialTexCoordSystem::ParaxialTexCoordSystem(const Vec3& point1, const Vec3& point2, const Vec3& point3) {
            const Vec3 normal = crossed(point3 - point1, point2 - point1).normalized();
            setRotation(normal, 0.0f, 0.0f);
        }
        
        size_t ParaxialTexCoordSystem::planeNormalIndex(const Vec3& normal) {
            size_t bestIndex = 0;
            FloatType bestDot = static_cast<FloatType>(0.0);
            for (size_t i = 0; i < 6; ++i) {
                const FloatType dot = normal.dot(BaseAxes[i * 3]);
                if (dot > bestDot) { // no need to use -altaxis for qbsp
                    bestDot = dot;
                    bestIndex = i;
                }
            }
            return bestIndex;
        }
        
        void ParaxialTexCoordSystem::axes(const size_t index, Vec3& xAxis, Vec3& yAxis) {
            Vec3 temp;
            axes(index, xAxis, yAxis, temp);
        }
        
        void ParaxialTexCoordSystem::axes(size_t index, Vec3& xAxis, Vec3& yAxis, Vec3& projectionAxis) {
            xAxis = BaseAxes[index * 3 + 1];
            yAxis = BaseAxes[index * 3 + 2];
            projectionAxis = BaseAxes[(index / 2) * 6];
        }
        
        TexCoordSystem* ParaxialTexCoordSystem::doClone() const {
            return new ParaxialTexCoordSystem(*this);
        }
        
        Vec3 ParaxialTexCoordSystem::getXAxis() const {
            return m_xAxis;
        }
        
        Vec3 ParaxialTexCoordSystem::getYAxis() const {
            return m_yAxis;
        }
        
        Vec3 ParaxialTexCoordSystem::getZAxis() const {
            return BaseAxes[m_index * 3 + 0];
        }
        
        bool ParaxialTexCoordSystem::isRotationInverted(const Vec3& normal) const {
            const size_t index = planeNormalIndex(normal);
            switch (index) {
                case 0:
                    return true;
                case 1:
                    return false;
                case 2:
                    return true;
                case 3:
                    return false;
                case 4: // Y axis rotation is the other way around (see rotateAxes method, too)
                    return false;
                default:
                    return true;
            }
        }
        
        Vec2f ParaxialTexCoordSystem::doGetTexCoords(const Vec3& point, const BrushFaceAttribs& attribs) const {
            const Vec2f texSize = attribs.textureSize();
            const Vec2f coords = computeTexCoords(point, attribs.scale());
            return (coords + attribs.offset()) / texSize;
        }
        
        void ParaxialTexCoordSystem::doSetRotation(const Vec3& normal, const float oldAngle, const float newAngle) {
            m_index = planeNormalIndex(normal);
            axes(m_index, m_xAxis, m_yAxis);
            rotateAxes(m_xAxis, m_yAxis, Math::radians(newAngle), m_index);
        }
        
        void ParaxialTexCoordSystem::doTransform(const Plane3& oldBoundary, const Mat4x4& transformation, BrushFaceAttribs& attribs, bool lockTexture) {
            // calculate the current texture coordinates of the origin
            const Vec3 offset     = transformation * Vec3::Null;
            const Vec3& oldNormal = oldBoundary.normal;
                  Vec3 newNormal  = transformation * oldNormal - offset;
            assert(Math::eq(newNormal.length(), 1.0));

            // fix some rounding errors - if the old and new texture axes are almost the same, use the old axis
            if (newNormal.equals(oldNormal, 0.01))
                newNormal = oldNormal;
            
            if (!lockTexture || attribs.xScale() == 0.0f || attribs.yScale() == 0.0f) {
                setRotation(newNormal, attribs.rotation(), attribs.rotation());
                return;
            }
            
            // calculate the current texture coordinates of the origin
            const Vec3 oldAnchor = oldBoundary.anchor();
            const Vec3 newAnchor = transformation * oldAnchor;
            const Vec2f oldAnchorTexCoords = computeTexCoords(oldAnchor, attribs.scale()) + attribs.offset();
            
            const Mat4x4 toBoundary        = planeProjectionMatrix4(0.0, oldNormal, zAxis());
            const Mat4x4 fromBoundary      = invertedMatrix(toBoundary);
            const Mat4x4 projectToBoundary = fromBoundary * Mat4x4::ZerZ * toBoundary;
            
            // compensate the translational part of the transformation for the directional vectors
            const Vec3 newXAxisOnBoundary = transformation * projectToBoundary * (m_xAxis * attribs.xScale()) - offset;
            const Vec3 newYAxisOnBoundary = transformation * projectToBoundary * (m_yAxis * attribs.yScale()) - offset;

            
            // fix some rounding errors - if the old and new texture axes are almost the same, use the old axis
            if (newNormal.equals(oldNormal, 0.01))
                newNormal = oldNormal;
            
            // obtain the new texture plane norm and the new base texture axes
            Vec3 newBaseXAxis, newBaseYAxis, newProjectionAxis;
            const size_t newIndex = planeNormalIndex(newNormal);
            axes(newIndex, newBaseXAxis, newBaseYAxis, newProjectionAxis);
            
            // project the transformed texture axes onto the new texture projection plane
            const Mat4x4 toTexPlane        = planeProjectionMatrix4(0.0, newProjectionAxis);
            const Mat4x4 fromTexPlane      = invertedMatrix(toTexPlane);
            const Mat4x4 projectToTexPlane = fromTexPlane * Mat4x4::ZerZ * toTexPlane;
            Vec3 newXAxis = projectToTexPlane * newXAxisOnBoundary;
            Vec3 newYAxis = projectToTexPlane * newYAxisOnBoundary;
            
            assert(!newXAxis.nan() && !newYAxis.nan());
            
            // the new scaling factors are the lengths of the transformed texture axes
            Vec2f newScale = Vec2f(newXAxis.length(),
                                   newYAxis.length());
            
            // normalize the transformed texture axes
            newXAxis /= newScale.x();
            newYAxis /= newScale.y();
            
            // WARNING: the texture plane norm is not the rotation axis of the texture (it's always the absolute axis)
            
            // determine the rotation angle from the dot product of the new base axes and the transformed texture axes
            float cosX = static_cast<float>(newBaseXAxis.dot(newXAxis));
            assert(!Math::isnan(cosX));
            float radX = std::acos(cosX);
            if (crossed(newBaseXAxis, newXAxis).dot(newProjectionAxis) < 0.0)
                radX *= -1.0f;
            
            // for some reason, when the texture plane normal is the Y axis, we must rotation clockwise
            float rad = radX;
            if (newIndex == 4)
                rad *= -1.0f;
            
            float newRotation = Math::degrees(rad);
            newRotation = Math::correct(newRotation, 4);
            
            // apply the rotation to the new base axes
            rotateAxes(newBaseXAxis, newBaseYAxis, rad, newIndex);
            
            // the sign of the scaling factors depends on the angle between the new base axis and the new texture axis
            if (newBaseXAxis.dot(newXAxis) < 0.0)
                newScale[0] *= -1.0f;
            if (newBaseYAxis.dot(newYAxis) < 0.0)
                newScale[1] *= -1.0f;
            
            // correct rounding errors
            newScale.correct(4);
            
            doSetRotation(newNormal, attribs.rotation(), newRotation);
            
            // determine the new texture coordinates of the transformed center of the face, sans offsets
            const Vec2f newAnchorTexCoords = computeTexCoords(newAnchor, newScale);
            
            // since the center should be invariant, the offsets are determined by the difference of the current and
            // the original texture coordinates of the center
            Vec2f newOffset = oldAnchorTexCoords - newAnchorTexCoords;
            modOffset(newOffset, attribs.texture());
            newOffset.correct(4);
            
            assert(!newOffset.nan());
            assert(!newScale.nan());
            assert(!Math::isnan(newRotation));
            assert(!Math::zero(newScale.x()));
            assert(!Math::zero(newScale.y()));
            
            attribs.setOffset(newOffset);
            attribs.setScale(newScale);
            attribs.setRotation(newRotation);
        }
        
        void ParaxialTexCoordSystem::doTransform(const Plane3& boundary, const Mat2x2& transform, BrushFaceAttribs& attribs, const Vec3& invariant) {
            const Vec2 xAxis = Vec2::PosX;
            const Vec2 yAxis = Vec2::NegY;
            
            const Vec2 offset   = transform * Vec2::Null;
            const Vec2 newXAxis = transform * xAxis - offset;
            const Vec2 newYAxis = transform * yAxis - offset;
            
            const Vec2 scaleDelta(newXAxis.length(),
                                  newYAxis.length());
            
            const float oldRotation = attribs.rotation();
            const FloatType angleDelta = Math::degrees(angleBetween(newXAxis.normalized(), xAxis));

            const float newRotation = Math::correct(oldRotation + float(angleDelta), 4);
            const Vec2f newScale = (attribs.scale() * Vec2f(scaleDelta)).corrected(4);
            
            const Vec2f oldCoords = getTexCoords(invariant, attribs) * attribs.textureSize();
            
//            attribs.setOffset(attribs.offset() + Vec2f(offset));
            attribs.setRotation(newRotation);
            attribs.setScale(newScale);
            
            doSetRotation(boundary.normal, oldRotation, newRotation);
            
            const Vec2f oldOffset = attribs.offset();
            const Vec2f newCoords = getTexCoords(invariant, attribs) * attribs.textureSize();
            const Vec2f newOffset = (oldOffset + oldCoords - newCoords).corrected(4);
            attribs.setOffset(newOffset);
        }
        
        float ParaxialTexCoordSystem::doMeasureAngle(const float currentAngle, const Vec2f& center, const Vec2f& point) const {
            const Vec3& zAxis = m_index % 2 == 0 ? Vec3::PosZ : Vec3::NegZ;
            const Quat3 rot(zAxis, -Math::radians(currentAngle));
            const Vec3 vec = rot * (point - center);
            
            const FloatType angleInRadians = Math::C::twoPi() - angleBetween(vec.normalized(), Vec3::PosX, zAxis);
            return static_cast<float>(Math::degrees(angleInRadians));
        }
        
        void ParaxialTexCoordSystem::rotateAxes(Vec3& xAxis, Vec3& yAxis, const FloatType angleInRadians, const size_t planeNormIndex) const {
            const Quat3 rot(BaseAxes[(planeNormIndex / 2) * 6], planeNormIndex == 4 ? -angleInRadians : angleInRadians);
            xAxis = rot * xAxis;
            yAxis = rot * yAxis;
        }
    }
}
