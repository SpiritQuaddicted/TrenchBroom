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

#include "AddObjectsQuery.h"

#include "CollectionUtils.h"
#include "Model/Brush.h"
#include "Model/Entity.h"
#include "Model/Group.h"
#include "Model/RemoveObjectsQuery.h"

namespace TrenchBroom {
    namespace Model {
        AddObjectsQuery::AddObjectsQuery() {}
        
        AddObjectsQuery::AddObjectsQuery(const RemoveObjectsQuery& removeQuery) {
            const EntityList& removedEntities = removeQuery.entities();
            EntityList::const_iterator eIt, eEnd;
            for (eIt = removedEntities.begin(), eEnd = removedEntities.end(); eIt != eEnd; ++eIt) {
                Entity* entity = *eIt;
                Layer* layer = entity->layer();
                Group* group = entity->group();
                addEntity(entity, layer, group);
            }
            
            const BrushList& removedBrushes = removeQuery.brushes();
            BrushList::const_iterator bIt, bEnd;
            for (bIt = removedBrushes.begin(), bEnd = removedBrushes.end(); bIt != bEnd; ++bIt) {
                Brush* brush = *bIt;
                Entity* entity = brush->parent();
                Layer* layer = brush->layer();
                Group* group = brush->group();
                addBrush(brush, entity, layer, group);
            }
        }

        const ObjectList& AddObjectsQuery::parents() const {
            return m_parents;
        }
        
        const ObjectList& AddObjectsQuery::objects() const {
            return m_objects;
        }
        
        size_t AddObjectsQuery::objectCount() const {
            return m_objects.size();
        }

        const EntityList& AddObjectsQuery::entities() const {
            return m_entities;
        }
        
        const EntityBrushesMap& AddObjectsQuery::brushes() const {
            return m_brushes;
        }
        
        const GroupList& AddObjectsQuery::groups() const {
            return m_groups;
        }
        
        const ObjectGroupMap& AddObjectsQuery::objectGroups() const {
            return m_objectGroups;
        }

        const ObjectLayerMap& AddObjectsQuery::objectLayers() const {
            return m_objectLayers;
        }

        void AddObjectsQuery::addGroup(Group* group, Layer* layer, Group* container) {
            assert(group != NULL);
            assert(layer != NULL);
            assert(checkGroup(group, layer, container));
            assert(!VectorUtils::contains(m_groups, group));
            assert(!VectorUtils::contains(m_objects, group));
            
            setLayer(group, layer);
            setGroup(group, container);
            m_groups.push_back(group);
            m_objects.push_back(group);
        }

        void AddObjectsQuery::addEntity(Entity* entity, Layer* layer, Group* group) {
            assert(entity != NULL);
            assert(layer != NULL);
            assert(checkEntity(entity, layer, group));
            assert(!VectorUtils::contains(m_entities, entity));
            assert(!VectorUtils::contains(m_objects, entity));
            
            setLayer(entity, layer);
            setGroup(entity, group);
            m_entities.push_back(entity);
            m_objects.push_back(entity);
        }
        
        void AddObjectsQuery::addBrushes(const EntityBrushesMap& brushes, const ObjectLayerMap& layers, const ObjectGroupMap& groups) {
            EntityBrushesMap::const_iterator it, end;
            for (it = brushes.begin(), end = brushes.end(); it != end; ++it)
                addBrushes(it->second, it->first, layers, groups);
        }
        
        void AddObjectsQuery::addBrushes(const BrushList& brushes, Entity* entity, const ObjectLayerMap& layers, const ObjectGroupMap& groups) {
            BrushList::const_iterator it, end;
            for (it = brushes.begin(), end = brushes.end(); it != end; ++it) {
                Brush* brush = *it;
                Layer* layer = MapUtils::find(layers, brush, static_cast<Layer*>(NULL));
                Group* group = MapUtils::find(groups, brush, static_cast<Group*>(NULL));
                addBrush(brush, entity, layer, group);
            }
        }

        void AddObjectsQuery::addBrushes(const BrushList& brushes, Entity* entity, Model::Layer* layer, Model::Group* group) {
            BrushList::const_iterator it, end;
            for (it = brushes.begin(), end = brushes.end(); it != end; ++it) {
                Brush* brush = *it;
                addBrush(brush, entity, layer, group);
            }
        }

        void AddObjectsQuery::addBrush(Brush* brush, Entity* entity, Layer* layer, Group* group) {
            assert(brush != NULL);
            assert(entity != NULL);
            assert(layer != NULL);
            assert(checkBrush(brush, entity, layer, group));
            assert(!VectorUtils::contains(m_objects, brush));
            
            EntityBrushesMap::iterator it = MapUtils::findOrInsert(m_brushes, entity);
            BrushList& brushes = it->second;
            assert(!VectorUtils::contains(brushes, brush));

            if (brushes.empty()) {
                assert(!VectorUtils::contains(m_parents, entity));
                m_parents.push_back(entity);
            }
            
            setLayer(brush, layer);
            setGroup(brush, group);
            brushes.push_back(brush);
            m_objects.push_back(brush);
        }

        void AddObjectsQuery::setLayer(Object* object, Layer* layer) {
            assert(m_objectLayers.count(object) == 0);
            m_objectLayers[object] = layer;
        }
        
        void AddObjectsQuery::setGroup(Object* object, Group* group) {
            assert(m_objectGroups.count(object) == 0);
            m_objectGroups[object] = group;
        }

        bool AddObjectsQuery::checkGroup(Group* group, Layer* layer, Group* container) const {
            if (!checkObjectGroup(layer, container))
                return false;
            return true;
        }

        bool AddObjectsQuery::checkEntity(Entity* entity, Layer* layer, Group* group) const {
            if (!checkObjectGroup(layer, group))
                return false;
            return true;
        }

        bool AddObjectsQuery::checkBrush(Brush* brush, Entity* entity, Layer* layer, Group* group) const {
            if (!checkObjectGroup(layer, group))
                return false;
            if (!checkBrushLayer(brush, entity, layer))
                return false;
            if (!checkBrushGroup(brush, entity, group))
                return false;
            return true;
        }

        bool AddObjectsQuery::checkBrushLayer(Brush* brush, Entity* entity, Layer* layer) const {
            if (!entity->worldspawn() && layer != getLayer(entity))
                return false;
            return true;
        }
        
        bool AddObjectsQuery::checkBrushGroup(Brush* brush, Entity* entity, Group* group) const {
            if (getLayer(entity) != getLayer(group))
                return false;
            if (!entity->worldspawn() && group != getGroup(entity))
                return true;
            return false;
        }
        
        bool AddObjectsQuery::checkObjectGroup(Layer* layer, Group* group) const {
            if (group == NULL)
                return true;
            if (layer != getLayer(group))
                return false;
            return checkObjectGroup(layer, getGroup(group));
        }

        Layer* AddObjectsQuery::getLayer(Object* object) const {
            if (object == NULL)
                return NULL;
            Layer* layer = object->layer();
            if (layer != NULL)
                return layer;
            assert(VectorUtils::contains(m_objects, object));
            return MapUtils::find(m_objectLayers, object, static_cast<Layer*>(NULL));
        }
        
        Group* AddObjectsQuery::getGroup(Object* object) const {
            if (object == NULL)
                return NULL;
            Group* group = object->group();
            if (group != NULL)
                return group;
            return MapUtils::find(m_objectGroups, object, static_cast<Group*>(NULL));
        }
        
        void AddObjectsQuery::clear() {
            m_parents.clear();
            m_objects.clear();
            m_groups.clear();
            m_entities.clear();
            m_brushes.clear();
            m_objectLayers.clear();
            m_objectGroups.clear();
        }
        
        void AddObjectsQuery::clearAndDelete() {
            m_parents.clear();
            m_objects.clear();
            VectorUtils::clearAndDelete(m_groups);
            VectorUtils::clearAndDelete(m_entities);
            MapUtils::clearAndDelete(m_brushes);
            m_objectLayers.clear();
            m_objectGroups.clear();
        }

        AddObjectsQueryBuilder::AddObjectsQueryBuilder(Layer* layer, Group* group, Entity* entity) :
        m_layer(layer),
        m_group(group),
        m_entity(entity) {}
        
        const AddObjectsQuery& AddObjectsQueryBuilder::query() const {
            return m_query;
        }
        
        void AddObjectsQueryBuilder::setLayer(Layer* layer) {
            assert(layer != NULL);
            m_layer = layer;
        }
        
        void AddObjectsQueryBuilder::setGroup(Group* group) {
            m_group = group;
        }

        void AddObjectsQueryBuilder::setEntity(Entity* entity) {
            m_entity = entity;
        }

        void AddObjectsQueryBuilder::doVisit(Entity* entity) {
            m_query.addEntity(entity, m_layer, m_group);
        }
        
        void AddObjectsQueryBuilder::doVisit(Brush* brush) {
            m_query.addBrush(brush, m_entity, m_layer, m_group);
        }
    }
}
