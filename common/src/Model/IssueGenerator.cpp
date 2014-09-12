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

#include "IssueGenerator.h"

#include "Model/Brush.h"
#include "Model/Entity.h"
#include "Model/Object.h"

#include <cassert>

namespace TrenchBroom {
    namespace Model {
        IssueGenerator::~IssueGenerator() {}
        
        void IssueGenerator::generate(Group* group, IssueList& issues) const {}
        void IssueGenerator::generate(Entity* entity, IssueList& issues) const {}
        void IssueGenerator::generate(Brush* brush, IssueList& issues) const {}
    }
}
