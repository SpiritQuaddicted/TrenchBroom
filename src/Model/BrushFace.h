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
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TrenchBroom__Face__
#define __TrenchBroom__Face__

#include "TrenchBroom.h"
#include "VecMath.h"
#include "SharedPointer.h"
#include "StringUtils.h"
#include "Model/Texture.h"
#include "Renderer/Mesh.h"
#include "Renderer/VertexSpec.h"

#include <vector>

namespace TrenchBroom {
    namespace Model {
        class BrushFaceGeometry;
        class BrushFace;
        
        class TextureCoordinateSystem {
        private:
            static const Vec3 BaseAxes[18];
            
            BrushFace* m_face;
            mutable bool m_valid;
            mutable size_t m_texPlaneNormIndex;
            mutable size_t m_texFaceNormIndex;
            mutable Vec3 m_texAxisX;
            mutable Vec3 m_texAxisY;
            mutable Vec3 m_scaledTexAxisX;
            mutable Vec3 m_scaledTexAxisY;
        public:
            TextureCoordinateSystem(BrushFace* face);
            Vec2f textureCoordinates(const Vec3& vertex) const;
            void invalidate();
        private:
            void validate() const;
            void axesAndIndices(const Vec3& normal, Vec3& xAxis, Vec3& yAxis, size_t& planeNormIndex, size_t& faceNormIndex) const;
            void rotateAxes(Vec3& xAxis, Vec3& yAxis, const FloatType angle, const size_t planeNormIndex) const;
        };

        class BrushFace {
        public:
            /*
             * The order of points, when looking from outside the face:
             *
             * 0-----------1
             * |
             * |
             * |
             * |
             * 2
             */
            typedef Vec3 Points[3];
            
            typedef std::tr1::shared_ptr<BrushFace> Ptr;
            typedef std::tr1::weak_ptr<BrushFace> WkPtr;
            typedef std::vector<BrushFace::Ptr> List;
            static const List EmptyList;

            typedef Renderer::VertexSpecs::P3NT2 VertexSpec;
            typedef VertexSpec::VertexType Vertex;
            typedef Renderer::Mesh<Texture::Ptr, VertexSpec> Mesh;
            static const String NoTextureName;
        private:
            BrushFace::Points m_points;
            Plane3 m_boundary;
            String m_textureName;
            float m_xOffset;
            float m_yOffset;
            float m_rotation;
            float m_xScale;
            float m_yScale;
            size_t m_lineNumber;
            size_t m_lineCount;
            
            Texture::Ptr m_texture;
            BrushFaceGeometry* m_side;
            TextureCoordinateSystem m_textureCoordinateSystem;
        private:
            BrushFace(const Vec3& point0, const Vec3& point1, const Vec3& point2, const String& textureName);
        public:
            static BrushFace::Ptr newBrushFace(const Vec3& point0, const Vec3& point1, const Vec3& point2, const String& textureName = NoTextureName);
            
            const BrushFace::Points& points() const;
            bool arePointsOnPlane(const Plane3& plane) const;
            
            const String& textureName() const;
            Texture::Ptr texture() const;
            const Plane3& boundary() const;
            float xOffset() const;
            float yOffset() const;
            float rotation() const;
            float xScale() const;
            float yScale() const;
            
            void setTexture(Texture::Ptr texture);
            void setXOffset(const float xOffset);
            void setYOffset(const float yOffset);
            void setRotation(const float rotation);
            void setXScale(const float xScale);
            void setYScale(const float yScale);
            void setFilePosition(const size_t lineNumber, const size_t lineCount);
            void setSide(BrushFaceGeometry* side);
            
            void addToMesh(Mesh& mesh);
        private:
            void setPoints(const Vec3& point0, const Vec3& point1, const Vec3& point2);
        };
    }
}

#endif /* defined(__TrenchBroom__Face__) */
