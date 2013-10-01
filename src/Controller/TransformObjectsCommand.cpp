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

#include "TransformObjectsCommand.h"

#include "Model/ModelUtils.h"
#include "View/MapDocument.h"

#include <cassert>

namespace TrenchBroom {
    namespace Controller {
        const Command::CommandType TransformObjectsCommand::Type = Command::freeType();

        TransformObjectsCommand::Ptr TransformObjectsCommand::transformObjects(View::MapDocumentPtr document, const Mat4x4& transformation, const bool lockTextures, const String& action, const Model::ObjectList& objects) {
            return Ptr(new TransformObjectsCommand(document, transformation, lockTextures, action, objects));
        }

        TransformObjectsCommand::TransformObjectsCommand(View::MapDocumentPtr document, const Mat4x4& transformation, const bool lockTextures, const String& action, const Model::ObjectList& objects) :
        Command(Type, makeName(action, objects), true, true),
        m_transformation(transformation),
        m_lockTextures(lockTextures),
        m_objects(objects) {
            assert(!objects.empty());
        }
        
        String TransformObjectsCommand::makeName(const String& action, const Model::ObjectList& objects) {
            return action + (objects.size() == 1 ? " object" : " objects");
        }
        
        bool TransformObjectsCommand::doPerformDo() {
            m_snapshot = Model::Snapshot(m_objects);
            Model::each(m_objects.begin(), m_objects.end(), Model::Transform(m_transformation, m_lockTextures, m_document->worldBounds()), Model::MatchAll());
            return true;
        }
        
        bool TransformObjectsCommand::doPerformUndo() {
            m_snapshot.restore(m_document->worldBounds());
            return true;
        }
    }
}