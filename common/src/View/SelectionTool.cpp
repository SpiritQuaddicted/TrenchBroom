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

#include "SelectionTool.h"

#include "Model/Brush.h"
#include "Model/HitAdapter.h"
#include "Model/Entity.h"
#include "Model/ModelFilter.h"
#include "Model/Object.h"
#include "Model/Picker.h"
#include "Renderer/RenderContext.h"
#include "View/ControllerFacade.h"
#include "View/InputState.h"
#include "View/MapDocument.h"

namespace TrenchBroom {
    namespace View {
        SelectionTool::SelectionTool(MapDocumentWPtr document, ControllerWPtr controller) :
        ToolImpl(document, controller) {}

        bool SelectionTool::doMouseUp(const InputState& inputState) {
            if (!inputState.mouseButtonsPressed(MouseButtons::MBLeft))
                return false;
            
            const bool multi = inputState.modifierKeysDown(ModifierKeys::MKCtrlCmd);
            const bool faces = inputState.modifierKeysDown(ModifierKeys::MKShift);
            
            if (faces) {
                const Hit& hit = Model::findFirstHit(inputState.hits(), Model::Brush::BrushHit, document()->filter(), true);
                if (hit.isMatch()) {
                    Model::BrushFace* face = Model::hitAsFace(hit);
                    if (multi) {
                        const bool objects = !document()->selectedObjects().empty();
                        if (objects) {
                            const Model::Brush* brush = face->parent();
                            if (brush->selected())
                                controller()->deselectFace(face);
                            else
                                controller()->selectFaceAndKeepBrushes(face);
                        } else {
                            if (face->selected())
                                controller()->deselectFace(face);
                            else
                                controller()->selectFace(face);
                        }
                    } else {
                        controller()->deselectAllAndSelectFace(face);
                    }
                } else {
                    controller()->deselectAll();
                }
            } else {
                const Hit& hit = Model::findFirstHit(inputState.hits(), Model::Entity::EntityHit | Model::Brush::BrushHit, document()->filter(), true);
                if (hit.isMatch()) {
                    Model::Object* object = Model::hitAsObject(hit);
                    if (multi) {
                        if (object->selected())
                            controller()->deselectObject(object);
                        else
                            controller()->selectObject(object);
                    } else {
                        controller()->deselectAllAndSelectObject(object);
                    }
                } else {
                    controller()->deselectAll();
                }
            }
            
            return true;
        }
        
        bool SelectionTool::doMouseDoubleClick(const InputState& inputState) {
            return false;
        }
        
        bool SelectionTool::doStartMouseDrag(const InputState& inputState) {
            return false;
        }
        
        bool SelectionTool::doMouseDrag(const InputState& inputState) {
            return false;
        }
        
        void SelectionTool::doEndMouseDrag(const InputState& inputState) {
        }
        
        void SelectionTool::doCancelMouseDrag(const InputState& inputState) {
        }

        void SelectionTool::doSetRenderOptions(const InputState& inputState, Renderer::RenderContext& renderContext) const {
            const Hit& hit = Model::findFirstHit(inputState.hits(), Model::Entity::EntityHit | Model::Brush::BrushHit, document()->filter(), true);
            if (hit.isMatch() && Model::hitAsObject(hit)->selected())
                renderContext.setShowSelectionGuide();
        }
    }
}