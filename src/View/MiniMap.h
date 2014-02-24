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

#ifndef __TrenchBroom__MiniMap__
#define __TrenchBroom__MiniMap__

#include "View/ViewTypes.h"

#include <wx/panel.h>

class wxSlider;
class wxWindow;

namespace TrenchBroom {
    namespace Renderer {
        class RenderResources;
    }

    namespace View {
        class MiniMapView;
        
        class MiniMap : public wxPanel {
        private:
            wxSlider* m_zSlider;
            MiniMapView* m_miniMapView;
        public:
            MiniMap(wxWindow* parent, View::MapDocumentWPtr document, Renderer::RenderResources& renderResources);
            
            void OnZSliderChanged(wxScrollEvent& event);
        private:
            void createGui(View::MapDocumentWPtr document, Renderer::RenderResources& renderResources);
            void bindEvents();
        };
    }
}

#endif /* defined(__TrenchBroom__MiniMap__) */
