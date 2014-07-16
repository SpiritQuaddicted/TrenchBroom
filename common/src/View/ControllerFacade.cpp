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

#include "ControllerFacade.h"

#include "CollectionUtils.h"
#include "TrenchBroomApp.h"
#include "Controller/AddRemoveObjectsCommand.h"
#include "Controller/EntityPropertyCommand.h"
#include "Controller/FaceAttributeCommand.h"
#include "Controller/FixPlanePointsCommand.h"
#include "Controller/MoveBrushEdgesCommand.h"
#include "Controller/MoveBrushFacesCommand.h"
#include "Controller/MoveBrushVerticesCommand.h"
#include "Controller/MoveTexturesCommand.h"
#include "Controller/NewDocumentCommand.h"
#include "Controller/OpenDocumentCommand.h"
#include "Controller/ReparentBrushesCommand.h"
#include "Controller/ResizeBrushesCommand.h"
#include "Controller/RotateTexturesCommand.h"
#include "Controller/SelectionCommand.h"
#include "Controller/SetEntityDefinitionFileCommand.h"
#include "Controller/SetModsCommand.h"
#include "Controller/SnapBrushVerticesCommand.h"
#include "Controller/SplitBrushEdgesCommand.h"
#include "Controller/SplitBrushFacesCommand.h"
#include "Controller/TextureCollectionCommand.h"
#include "Controller/TransformObjectsCommand.h"
#include "Controller/TransformTexCoordSystemCommand.h"
#include "Model/ModelUtils.h"
#include "View/MapDocument.h"
#include "View/ViewTypes.h"

#include <cassert>

namespace TrenchBroom {
    namespace View {
        UndoableCommandGroup::UndoableCommandGroup(ControllerSPtr controller, const String& name) :
        m_controller(controller.get()) {
            m_controller->beginUndoableGroup(name);
        }
        
        UndoableCommandGroup::UndoableCommandGroup(ControllerFacade* controller, const String& name) :
        m_controller(controller) {
            m_controller->beginUndoableGroup(name);
        }

        UndoableCommandGroup::~UndoableCommandGroup() {
            m_controller->closeGroup();
        }

        void UndoableCommandGroup::rollback() {
            m_controller->rollbackGroup();
        }

        ControllerFacade::MoveVerticesResult::MoveVerticesResult(bool i_success, bool i_hasRemainingVertices) :
        success(i_success),
        hasRemainingVertices(i_hasRemainingVertices) {
            assert(success || !hasRemainingVertices);
        }
        
        ControllerFacade::ControllerFacade(MapDocumentWPtr document) :
        m_document(document),
        commandDoNotifier(m_commandProcessor.commandDoNotifier),
        commandDoneNotifier(m_commandProcessor.commandDoneNotifier),
        commandDoFailedNotifier(m_commandProcessor.commandDoFailedNotifier),
        commandUndoNotifier(m_commandProcessor.commandUndoNotifier),
        commandUndoneNotifier(m_commandProcessor.commandUndoneNotifier),
        commandUndoFailedNotifier(m_commandProcessor.commandUndoFailedNotifier) {}
        
        bool ControllerFacade::hasLastCommand() const {
            return m_commandProcessor.hasLastCommand();
        }
        
        bool ControllerFacade::hasNextCommand() const {
            return m_commandProcessor.hasNextCommand();
        }
        
        const String& ControllerFacade::lastCommandName() const {
            return m_commandProcessor.lastCommandName();
        }
        
        const String& ControllerFacade::nextCommandName() const {
            return m_commandProcessor.nextCommandName();
        }

        bool ControllerFacade::newDocument(const BBox3& worldBounds, Model::GamePtr game, const Model::MapFormat::Type mapFormat) {
            using namespace Controller;
            
            Command::Ptr command = Command::Ptr(new NewDocumentCommand(m_document, worldBounds, game, mapFormat));
            return m_commandProcessor.submitCommand(command);
        }
        
        bool ControllerFacade::openDocument(const BBox3& worldBounds, Model::GamePtr game, const IO::Path& path) {
            using namespace Controller;
            
            Command::Ptr command = Command::Ptr(new OpenDocumentCommand(m_document, worldBounds, game, path));
            if (m_commandProcessor.submitCommand(command)) {
                View::TrenchBroomApp::instance().updateRecentDocument(path);
                return true;
            }
            return false;
        }

        void ControllerFacade::beginUndoableGroup(const String& name) {
            m_commandProcessor.beginUndoableGroup(name);
        }
        
        void ControllerFacade::beginOneShotGroup(const String& name) {
            m_commandProcessor.beginOneShotGroup(name);
        }
        
        void ControllerFacade::closeGroup() {
            m_commandProcessor.closeGroup();
        }

        void ControllerFacade::rollbackGroup() {
            m_commandProcessor.undoGroup();
        }

        bool ControllerFacade::undoLastCommand() {
            return m_commandProcessor.undoLastCommand();
        }
        
        bool ControllerFacade::redoNextCommand() {
            return m_commandProcessor.redoNextCommand();
        }

        bool ControllerFacade::selectObjects(const Model::ObjectList& objects) {
            using namespace Controller;
            
            Command::Ptr command = SelectionCommand::select(m_document, objects);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::selectObject(Model::Object* object) {
            using namespace Controller;
            
            Command::Ptr command = SelectionCommand::select(m_document, Model::ObjectList(1, object));
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::selectAllObjects() {
            using namespace Controller;
            
            Command::Ptr command = SelectionCommand::selectAllObjects(m_document);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::deselectAllAndSelectObjects(const Model::ObjectList& objects) {
            using namespace Controller;

            Command::Ptr deselectCommand = SelectionCommand::deselectAll(m_document);
            Command::Ptr selectCommand = SelectionCommand::select(m_document, objects);

            const UndoableCommandGroup commandGroup(this);
            m_commandProcessor.submitAndStoreCommand(deselectCommand);
            m_commandProcessor.submitAndStoreCommand(selectCommand);
            
            return true;
        }

        bool ControllerFacade::deselectAllAndSelectObject(Model::Object* object) {
            return deselectAllAndSelectObjects(Model::ObjectList(1, object));
        }
        
        bool ControllerFacade::deselectObject(Model::Object* object) {
            using namespace Controller;
            
            Command::Ptr command = SelectionCommand::deselect(m_document, Model::ObjectList(1, object));
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::selectFace(Model::BrushFace* face) {
            return selectFaces(Model::BrushFaceList(1, face));
        }
        
        bool ControllerFacade::selectFaces(const Model::BrushFaceList& faces) {
            using namespace Controller;
            
            Command::Ptr command = SelectionCommand::select(m_document, faces);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::selectFaceAndKeepBrushes(Model::BrushFace* face) {
            using namespace Controller;
            
            Command::Ptr command = SelectionCommand::selectAndKeepBrushes(m_document, Model::BrushFaceList(1, face));
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::deselectAllAndSelectFace(Model::BrushFace* face) {
            return deselectAllAndSelectFaces(Model::BrushFaceList(1, face));
        }
        
        bool ControllerFacade::deselectAllAndSelectFaces(const Model::BrushFaceList& faces) {
            using namespace Controller;
            
            Command::Ptr deselectCommand = SelectionCommand::deselectAll(m_document);
            Command::Ptr selectCommand = SelectionCommand::select(m_document, faces);
            
            const UndoableCommandGroup commandGroup(this);
            m_commandProcessor.submitAndStoreCommand(deselectCommand);
            m_commandProcessor.submitAndStoreCommand(selectCommand);
            
            return true;
        }

        bool ControllerFacade::deselectFace(Model::BrushFace* face) {
            using namespace Controller;

            Command::Ptr command = SelectionCommand::deselect(m_document, Model::BrushFaceList(1, face));
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::deselectAll() {
            using namespace Controller;

            Command::Ptr deselectCommand = SelectionCommand::deselectAll(m_document);
            return m_commandProcessor.submitAndStoreCommand(deselectCommand);
        }

        bool ControllerFacade::addEntity(Model::Entity* entity) {
            return addEntities(Model::EntityList(1, entity));
        }
        
        bool ControllerFacade::addBrush(Model::Brush* brush) {
            return addBrushes(Model::BrushList(1, brush));
        }

        bool ControllerFacade::addEntities(const Model::EntityList& entities) {
            return addObjects(Model::makeObjectParentList(entities));
        }
        
        bool ControllerFacade::addBrushes(const Model::BrushList& brushes) {
            return addObjects(Model::makeObjectParentList(brushes, lock(m_document)->worldspawn()));
        }

        bool ControllerFacade::addObjects(const Model::ObjectParentList& objects) {
            using namespace Controller;
            
            Command::Ptr command = AddRemoveObjectsCommand::addObjects(m_document, objects);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::removeObjects(const Model::ObjectList& objects) {
            return removeObjects(Model::makeObjectParentList(objects));
        }
        
        bool ControllerFacade::removeObjects(const Model::ObjectParentList& objects) {
            using namespace Controller;
            
            Command::Ptr command = AddRemoveObjectsCommand::removeObjects(m_document, objects);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::removeObject(Model::Object* object) {
            return removeObjects(Model::ObjectList(1, object));
        }

        Model::ObjectList ControllerFacade::duplicateObjects(const Model::ObjectList& objects, const BBox3& worldBounds) {
            Model::ObjectParentList duplicates;
            Model::ObjectList result;
            
            Model::ObjectList::const_iterator it, end;
            for (it = objects.begin(), end = objects.end(); it != end; ++it) {
                Model::Object* object = *it;
                Model::Object* parent = NULL;
                if (object->type() == Model::Object::Type_Brush)
                    parent = static_cast<Model::Brush*>(object)->parent();
                
                Model::Object* duplicate = object->clone(worldBounds);
                result.push_back(duplicate);
                duplicates.push_back(Model::ObjectParentPair(duplicate, parent));
            }
            
            if (!addObjects(duplicates))
                VectorUtils::clearAndDelete(result);
            return result;
        }

        bool ControllerFacade::deleteSelectedObjects() {
            View::MapDocumentSPtr document = lock(m_document);
            const Model::ObjectList objects = document->selectedObjects();
            assert(!objects.empty());
            
            const UndoableCommandGroup commandGroup(this, String("Delete ") + String(objects.size() == 1 ? "object" : "objects"));
            if (!deselectAll())
                return false;
            return removeObjects(objects);
        }

        bool ControllerFacade::moveBrushesToWorldspawn(const Model::BrushList& brushes) {
            return reparentBrushes(brushes, lock(m_document)->worldspawn());
        }

        bool ControllerFacade::reparentBrushes(const Model::BrushList& brushes, Model::Entity* newParent) {
            using namespace Controller;
            
            const UndoableCommandGroup commandGroup(this);

            ReparentBrushesCommand::Ptr command = ReparentBrushesCommand::reparent(m_document, brushes, newParent);
            const bool success = m_commandProcessor.submitAndStoreCommand(command);
            if (success) {
                const Model::EntityList& emptyEntities = command->emptyEntities();
                if (!emptyEntities.empty())
                    removeObjects(VectorUtils::cast<Model::Object*>(emptyEntities));
            }
            
            return success;
        }

        bool ControllerFacade::renameEntityProperty(const Model::EntityList& entities, const Model::PropertyKey& oldKey, const Model::PropertyKey& newKey, const bool force) {
            using namespace Controller;

            Command::Ptr command = EntityPropertyCommand::renameEntityProperty(m_document, entities, oldKey, newKey, force);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::setEntityProperty(const Model::EntityList& entities, const Model::PropertyKey& key, const Model::PropertyValue& newValue, const bool force) {
            using namespace Controller;
            
            Command::Ptr command = EntityPropertyCommand::setEntityProperty(m_document, entities, key, newValue, force);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setEntityProperty(Model::Entity& entity, const Model::PropertyKey& key, const Model::PropertyValue& newValue, const bool force) {
            return setEntityProperty(Model::EntityList(1, &entity), key, newValue, force);
        }

        bool ControllerFacade::removeEntityProperty(const Model::EntityList& entities, const Model::PropertyKey& key, const bool force) {
            using namespace Controller;

            Command::Ptr command = EntityPropertyCommand::removeEntityProperty(m_document, entities, key, force);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setMods(const StringList& mods) {
            using namespace Controller;
            
            Command::Ptr command = SetModsCommand::setMods(m_document, mods);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setEntityDefinitionFile(const Model::EntityDefinitionFileSpec& spec) {
            using namespace Controller;
            
            Command::Ptr command = SetEntityDefinitionFileCommand::setEntityDefinitionFileSpec(m_document, spec);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::addTextureCollection(const String& name) {
            using namespace Controller;

            Command::Ptr command = TextureCollectionCommand::add(m_document, name);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::removeTextureCollections(const StringList& names) {
            using namespace Controller;
            
            Command::Ptr command = TextureCollectionCommand::remove(m_document, names);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::moveTextureCollectionUp(const String& name) {
            using namespace Controller;
            
            Command::Ptr command = TextureCollectionCommand::moveUp(m_document, name);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::moveTextureCollectionDown(const String& name) {
            using namespace Controller;
            
            Command::Ptr command = TextureCollectionCommand::moveDown(m_document, name);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::moveObjects(const Model::ObjectList& objects, const Vec3& delta, const bool lockTextures) {
            using namespace Controller;
            
            Command::Ptr command = TransformObjectsCommand::translateObjects(m_document, delta, lockTextures, objects);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::rotateObjects(const Model::ObjectList& objects, const Vec3& center, const Vec3& axis, const FloatType angle, const bool lockTextures) {
            using namespace Controller;
            
            Command::Ptr command = TransformObjectsCommand::rotateObjects(m_document, center, axis, angle, lockTextures, objects);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::flipObjects(const Model::ObjectList& objects, const Vec3& center, const Math::Axis::Type axis, const bool lockTextures) {
            using namespace Controller;

            Command::Ptr command = TransformObjectsCommand::flipObjects(m_document, center, axis, lockTextures, objects);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::resizeBrushes(const Model::BrushFaceList& faces, const Vec3& delta, const bool lockTextures) {
            using namespace Controller;
            
            Command::Ptr command = ResizeBrushesCommand::resizeBrushes(m_document, faces, delta, lockTextures);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::snapPlanePoints(Model::Brush& brush) {
            using namespace Controller;
            
            Command::Ptr command = FixPlanePointsCommand::snapPlanePoints(m_document, Model::BrushList(1, &brush));
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::findPlanePoints(Model::Brush& brush) {
            using namespace Controller;
            
            Command::Ptr command = FixPlanePointsCommand::findPlanePoints(m_document, Model::BrushList(1, &brush));
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        ControllerFacade::MoveVerticesResult ControllerFacade::moveVertices(const Model::VertexToBrushesMap& vertices, const Vec3& delta) {
            using namespace Controller;
            
            MoveBrushVerticesCommand::Ptr command = MoveBrushVerticesCommand::moveVertices(m_document, vertices, delta);
            const bool success = m_commandProcessor.submitAndStoreCommand(command);
            return MoveVerticesResult(success, command->hasRemainingVertices());
        }

        bool ControllerFacade::moveEdges(const Model::VertexToEdgesMap& edges, const Vec3& delta) {
            using namespace Controller;
            
            Command::Ptr command = MoveBrushEdgesCommand::moveEdges(m_document, edges, delta);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::moveFaces(const Model::VertexToFacesMap& faces, const Vec3& delta) {
            using namespace Controller;
            
            Command::Ptr command = MoveBrushFacesCommand::moveFaces(m_document, faces, delta);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::splitEdges(const Model::VertexToEdgesMap& edges, const Vec3& delta) {
            using namespace Controller;
            
            Command::Ptr command = SplitBrushEdgesCommand::moveEdges(m_document, edges, delta);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::splitFaces(const Model::VertexToFacesMap& faces, const Vec3& delta) {
            using namespace Controller;
            
            Command::Ptr command = SplitBrushFacesCommand::moveFaces(m_document, faces, delta);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::snapVertices(const Model::VertexToBrushesMap& vertices, const size_t snapTo) {
            using namespace Controller;

            Command::Ptr command = SnapBrushVerticesCommand::snapVertices(m_document, vertices, snapTo);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::snapVertices(const Model::BrushList& brushes, const size_t snapTo) {
            using namespace Controller;
            
            Command::Ptr command = SnapBrushVerticesCommand::snapAllVertices(m_document, brushes, snapTo);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setTexture(const Model::BrushFaceList& faces, Assets::Texture* texture) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            command->setTexture(texture);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setFaceXOffset(const Model::BrushFaceList& faces, const float xOffset, const bool add) {
            using namespace Controller;

            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (add)
                command->addXOffset(xOffset);
            else
                command->setXOffset(xOffset);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::setFaceYOffset(const Model::BrushFaceList& faces, const float yOffset, const bool add) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (add)
                command->addYOffset(yOffset);
            else
                command->setYOffset(yOffset);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::setFaceOffset(const Model::BrushFaceList& faces, const Vec2f& offset, bool add) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (add) {
                command->addXOffset(offset.x());
                command->addYOffset(offset.y());
            } else {
                command->setXOffset(offset.x());
                command->setYOffset(offset.y());
            }
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setFaceRotation(const Model::BrushFaceList& faces, const float rotation, const bool add) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (add)
                command->addRotation(rotation);
            else
                command->setRotation(rotation);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::setFaceXScale(const Model::BrushFaceList& faces, const float xScale, const bool add) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (add)
                command->addXScale(xScale);
            else
                command->setXScale(xScale);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::setFaceYScale(const Model::BrushFaceList& faces, const float yScale, const bool add) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (add)
                command->addYScale(yScale);
            else
                command->setYScale(yScale);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::invertFaceXScale(const Model::BrushFaceList& faces) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            command->mulXScale(-1.0f);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::invertFaceYScale(const Model::BrushFaceList& faces) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            command->mulYScale(-1.0f);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setSurfaceFlag(const Model::BrushFaceList& faces, const size_t index, const bool set) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (set)
                command->setSurfaceFlag(index);
            else
                command->unsetSurfaceFlag(index);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::setContentFlag(const Model::BrushFaceList& faces, const size_t index, const bool set) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (set)
                command->setContentFlag(index);
            else
                command->unsetContentFlag(index);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
        
        bool ControllerFacade::setContentFlags(const Model::BrushFaceList& faces, int flags) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            command->replaceContentFlags(flags);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setSurfaceValue(const Model::BrushFaceList& faces, const float value, const bool add) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            if (add)
                command->addSurfaceValue(value);
            else
                command->setSurfaceValue(value);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::setFaceAttributes(const Model::BrushFaceList& faces, const Model::BrushFace& source) {
            using namespace Controller;
            
            FaceAttributeCommand::Ptr command(new FaceAttributeCommand(m_document, faces));
            command->setAll(source);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::moveTextures(const Model::BrushFaceList& faces, const Vec3& up, const Vec3& right, const Vec2f& offset) {
            using namespace Controller;
            
            Command::Ptr command = MoveTexturesCommand::moveTextures(m_document, faces, up, right, offset);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::rotateTextures(const Model::BrushFaceList& faces, const float angle) {
            using namespace Controller;
            
            Command::Ptr command = RotateTexturesCommand::rotateTextures(m_document, faces, angle);
            return m_commandProcessor.submitAndStoreCommand(command);
        }

        bool ControllerFacade::rotateTextures(const Model::BrushFaceList& faces, const FloatType angle, const Vec3& invariant) {
            using namespace Controller;
            
            Command::Ptr command = TransformTexCoordSystemCommand::rotateTextures(m_document, faces, angle, invariant);
            return m_commandProcessor.submitAndStoreCommand(command);
        }
    }
}
