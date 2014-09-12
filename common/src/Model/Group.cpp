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

#include "Group.h"

#include "Model/Brush.h"
#include "Model/Entity.h"

namespace TrenchBroom {
    namespace Model {
        const BBox3& Group::doGetBounds() const {
            m_bounds = Object::bounds(m_objects);
            return m_bounds;
        }

        void Group::doTransform(const Mat4x4& transformation, const bool lockTextures, const BBox3& worldBounds) {
            ObjectList::iterator it, end;
            for (it = m_objects.begin(), end = m_objects.end(); it != end; ++it) {
                Object* object = *it;
                object->transform(transformation, lockTextures, worldBounds);
            }
        }
        
        class GroupContains : public ConstObjectVisitor {
        private:
            const Group* m_this;
            bool m_result;
        public:
            GroupContains(const Group* i_this) :
            m_this(i_this),
            m_result(false) {}
            
            bool result() const {
                return m_result;
            }
            
            void doVisit(const Group* group) {
                m_result = m_this->bounds().contains(group->bounds());
            }
            
            void doVisit(const Entity* entity) {
                m_result = m_this->bounds().contains(entity->bounds());
            }
            
            void doVisit(const Brush* brush) {
                m_result = m_this->bounds().contains(brush->bounds());
            }
        };
        
        bool Group::doContains(const Object& object) const {
            GroupContains contains(this);
            object.accept(contains);
            return contains.result();
        }
        
        class GroupIntersects : public ConstObjectVisitor {
        private:
            const Group* m_this;
            bool m_result;
        public:
            GroupIntersects(const Group* i_this) :
            m_this(i_this),
            m_result(false) {}
            
            bool result() const {
                return m_result;
            }
            
            void doVisit(const Group* group) {
                m_result = m_this->bounds().intersects(group->bounds());
            }
            
            void doVisit(const Entity* entity) {
                m_result = m_this->bounds().intersects(entity->bounds());
            }
            
            void doVisit(const Brush* brush) {
                m_result = m_this->bounds().intersects(brush->bounds());
            }
        };
        
        bool Group::doIntersects(const Object& object) const {
            GroupIntersects intersects(this);
            object.accept(intersects);
            return intersects.result();
        }
        
        void Group::doAccept(ObjectVisitor& visitor) {
            visitor.visit(this);
        }
        
        void Group::doAccept(ConstObjectVisitor& visitor) const {
            visitor.visit(this);
        }
        
        void Group::doAcceptRecursively(ObjectVisitor& visitor) {
            visitor.visit(this);
            acceptRecursively(m_objects.begin(), m_objects.end(), visitor);
        }
        
        void Group::doAcceptRecursively(ConstObjectVisitor& visitor) const {
            visitor.visit(this);
            acceptRecursively(m_objects.begin(), m_objects.end(), visitor);
        }
    }
}
