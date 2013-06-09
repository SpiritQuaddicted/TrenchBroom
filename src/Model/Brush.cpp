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

#include "Brush.h"

#include "CollectionUtils.h"
#include "Model/BrushFace.h"
#include "Model/BrushGeometry.h"

namespace TrenchBroom {
    namespace Model {
        const Brush::List Brush::EmptyList = Brush::List();

        Brush::Brush(const BBox3& worldBounds, const BrushFace::List& faces) :
        m_faces(faces),
        m_geometry(NULL) {
            rebuildGeometry(worldBounds);
        }

        Brush::Ptr Brush::newBrush(const BBox3& worldBounds, const BrushFace::List& faces) {
            return Brush::Ptr(new Brush(worldBounds, faces));
        }
        
        Brush::~Brush() {
            delete m_geometry;
            m_geometry = NULL;
        }

        void Brush::rebuildGeometry(const BBox3& worldBounds) {
            delete m_geometry;
            m_geometry = new BrushGeometry(worldBounds, m_faces);
        }
    }
}