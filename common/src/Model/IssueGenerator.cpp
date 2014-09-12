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

        class IssueGeneratorVisitor : public ObjectVisitor {
        private:
            const IssueGenerator& m_generator;
            IssueList& m_issues;
        public:
            IssueGeneratorVisitor(const IssueGenerator& generator, IssueList& issues) :
            m_generator(generator),
            m_issues(issues) {}
        private:
            void doVisit(Group* group) {
                m_generator.generate(group, m_issues);
            }
            
            void doVisit(Entity* entity) {
                m_generator.generate(entity, m_issues);
            }
            
            void doVisit(Brush* brush) {
                m_generator.generate(brush, m_issues);
            }
        };
        
        void IssueGenerator::generate(Object* object, IssueList& issues) const {
            IssueGeneratorVisitor visitor(*this, issues);
            object->accept(visitor);
        }
        
        void IssueGenerator::generate(Group* group, IssueList& issues) const {
            doGenerate(group, issues);
        }
        
        void IssueGenerator::generate(Entity* entity, IssueList& issues) const {
            doGenerate(entity, issues);
        }
        
        void IssueGenerator::generate(Brush* brush, IssueList& issues) const {
            doGenerate(brush, issues);
        }

        void IssueGenerator::doGenerate(Group* group, IssueList& issues) const {}
        void IssueGenerator::doGenerate(Entity* entity, IssueList& issues) const {}
        void IssueGenerator::doGenerate(Brush* brush, IssueList& issues) const {}
    }
}
