/*
 Copyright (C) 2010-2013 Kristian Duske
 
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

#ifndef __TrenchBroom__RotateToolHelper__
#define __TrenchBroom__RotateToolHelper__

#include "StringUtils.h"
#include "TrenchBroom.h"
#include "VecMath.h"
#include "Renderer/Vbo.h"
#include "Renderer/TextRenderer.h"
#include "View/Tool.h"

namespace TrenchBroom {
    namespace Renderer {
        class RenderContext;
        class TextureFont;
    }
    
    namespace View {
        class InputState;
        
        struct RotateInfo {
            Vec3 center;
            Vec3 axis;
            Vec3 origin;
            Plane3 plane;
        };
        
        class RotateDelegate {
        public:
            virtual ~RotateDelegate();
            
            bool handleRotate(const InputState& inputState) const;
            RotateInfo getRotateInfo(const InputState& inputState) const;
            bool startRotate(const InputState& inputState);
            FloatType getAngle(const InputState& inputState, const Vec3& handlePoint, const Vec3& curPoint, const Vec3& axis) const;
            bool rotate(const Vec3& center, const Vec3& axis, const FloatType angle);
            void endRotate(const InputState& inputState);
            void cancelRotate(const InputState& inputState);
        private:
            virtual bool doHandleRotate(const InputState& inputState) const = 0;
            virtual RotateInfo doGetRotateInfo(const InputState& inputState) const = 0;
            virtual bool doStartRotate(const InputState& inputState) = 0;
            virtual FloatType doGetAngle(const InputState& inputState, const Vec3& handlePoint, const Vec3& curPoint, const Vec3& axis) const = 0;
            virtual bool doRotate(const Vec3& center, const Vec3& axis, const FloatType angle) = 0;
            virtual void doEndRotate(const InputState& inputState) = 0;
            virtual void doCancelRotate(const InputState& inputState) = 0;
        };
        
        class RotateHelper : public PlaneDragHelper {
        public:
            typedef Renderer::TextRenderer<size_t> TextRenderer;
        private:
            static const size_t SnapAngleKey;
            static const size_t AngleKey;
            
            RotateDelegate& m_delegate;
            Vec3 m_center;
            Vec3 m_axis;
            FloatType m_lastAngle;
            Vec3 m_firstPoint;
            Renderer::Vbo m_vbo;
            TextRenderer m_textRenderer;
        public:
            RotateHelper(RotateDelegate& delegate, Renderer::TextureFont& font);
            
            bool startPlaneDrag(const InputState& inputState, Plane3& plane, Vec3& initialPoint);
            bool planeDrag(const InputState& inputState, const Vec3& lastPoint, const Vec3& curPoint, Vec3& refPoint);
            void endPlaneDrag(const InputState& inputState);
            void cancelPlaneDrag(const InputState& inputState);
            void resetPlane(const InputState& inputState, Plane3& plane, Vec3& initialPoint);
            void render(const InputState& inputState, const bool dragging, Renderer::RenderContext& renderContext);
            
        private:
            void renderAngleIndicator(Renderer::RenderContext& renderContext);
            void renderText(Renderer::RenderContext& renderContext);
            String angleString(const FloatType angle) const;
            Renderer::TextAnchor::Ptr angleAnchor() const;
        };
    }
}

#endif /* defined(__TrenchBroom__RotateToolHelper__) */