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

#ifndef __TrenchBroom__TexturingViewHelper__
#define __TrenchBroom__TexturingViewHelper__

#include "TrenchBroom.h"
#include "VecMath.h"

namespace TrenchBroom {
    class Hits;
    
    namespace Assets {
        class Texture;
    }

    namespace Model {
        class BrushFace;
    }
    
    namespace Renderer {
        class ActiveShader;
        class OrthographicCamera;
    }

    namespace View {
        class TexturingViewHelper {
        private:
            Model::BrushFace* m_face;
            Vec3 m_origin;
            Vec3 m_xAxis;
            Vec3 m_yAxis;
            Vec3 m_zAxis;
            Mat4x4 m_toFaceTransform;
            Mat4x4 m_fromFaceTransform;
            
            float m_cameraZoom;
        public:
            TexturingViewHelper();
            
            bool valid() const;
            Model::BrushFace* face() const;
            const Assets::Texture* texture() const;
            
            const Vec3& origin() const;
            const Vec3& xAxis() const;
            const Vec3& yAxis() const;
            const Vec3& zAxis() const;
            
            BBox3 computeBounds() const;
            Vec3 transformToFace(const Vec3& point) const;
            Vec3 transformFromFace(const Vec3& point) const;
            Vec2f textureCoords(const Vec3f& point) const;
            
            Vec2f snapOffset(const Vec2f& delta) const;
            
            Vec3::List textureSeamVertices(const Renderer::OrthographicCamera& camera) const;
            Mat4x4 worldToTexMatrix() const;
            
            void activateTexture(Renderer::ActiveShader& shader);
            void deactivateTexture();
            
            Hits pick(const Ray3& pickRay) const;
            
            void setFace(Model::BrushFace* face);
            void setCameraZoom(float cameraZoom);
        private:
            void validate();
        };
    }
}

#endif /* defined(__TrenchBroom__TexturingViewHelper__) */