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

#include "MapWriter.h"

#include "Exceptions.h"
#include "IO/DiskFileSystem.h"
#include "IO/Path.h"
#include "Model/Brush.h"
#include "Model/Entity.h"
#include "Model/Issue.h"
#include "Model/Layer.h"
#include "Model/Map.h"
#include "Model/Object.h"

#include <cassert>
#include <fstream>

namespace TrenchBroom {
    namespace IO {
        MapWriter::~MapWriter() {}
        
        struct CategorizeEntities : public Model::ObjectVisitor {
            Model::Entity* worldspawn;
            Model::EntityList pointEntities;
            Model::EntityBrushesMap brushEntities;
            
            CategorizeEntities() :
            worldspawn(NULL) {}
            
            void doVisit(Model::Entity* entity) {
                pointEntities.push_back(entity);
            }
            
            void doVisit(Model::Brush* brush) {
                Model::Entity* entity = brush->parent();
                brushEntities[entity].push_back(brush);
                if (entity->worldspawn())
                    worldspawn = entity;
            }
            
            void doVisit(Model::Group* group) {}
        };
        
        void MapWriter::writeObjectsToStream(const Model::ObjectList& objects, std::ostream& stream) {
            assert(stream.good());
            stream.unsetf(std::ios::floatfield);

            CategorizeEntities toWrite;
            Model::Object::accept(objects.begin(), objects.end(), toWrite);
            
            // write worldspawn first
            Model::BrushList::const_iterator bIt, bEnd;
            if (toWrite.worldspawn != NULL)
                writeEntity(toWrite.worldspawn, toWrite.brushEntities[toWrite.worldspawn], stream);
            
            // write point entities
            Model::EntityList::const_iterator eIt, eEnd;
            for (eIt = toWrite.pointEntities.begin(), eEnd = toWrite.pointEntities.end(); eIt != eEnd; ++eIt) {
                Model::Entity* entity = *eIt;
                const Model::BrushList& brushes = entity->brushes(); // should be empty, but you never know
                writeEntity(entity, brushes, stream);
            }
            
            // write brush entities except for worldspawn
            Model::EntityBrushesMap::const_iterator ebIt, ebEnd;
            for (ebIt = toWrite.brushEntities.begin(), ebEnd = toWrite.brushEntities.end(); ebIt != ebEnd; ++ebIt) {
                Model::Entity* entity = ebIt->first;
                if (entity != toWrite.worldspawn) {
                    const Model::BrushList& brushes = ebIt->second;
                    writeEntity(entity, brushes, stream);
                }
            }
        }
        
        void MapWriter::writeFacesToStream(const Model::BrushFaceList& faces, std::ostream& stream) {
            assert(stream.good());
            stream.unsetf(std::ios::floatfield);
            
            Model::BrushFaceList::const_iterator it, end;
            for (it = faces.begin(), end = faces.end(); it != end; ++it)
                writeFace(*it, stream);
        }
        
        void MapWriter::writeToStream(const Model::Map* map, std::ostream& stream) {
            assert(map != NULL);
            assert(stream.good());
            stream.unsetf(std::ios::floatfield);
            
            const Model::EntityList& entities = map->entities();
            Model::EntityList::const_iterator it, end;
            for (it = entities.begin(), end = entities.end(); it != end; ++it) {
                Model::Entity* entity = *it;
                const Model::BrushList& brushes = entity->brushes();
                writeEntity(entity, brushes, stream);
            }
        }
        
        void MapWriter::writeToFileAtPath(Model::Map* map, const Path& path, const bool overwrite) {
            assert(map != NULL);
            
            if (IO::Disk::fileExists(IO::Disk::fixPath(path)) && !overwrite)
                throw FileSystemException("File already exists: " + path.asString());
            
            // ensure that the directory actually exists or is created if it doesn't
            const Path directoryPath = path.deleteLastComponent();
            WritableDiskFileSystem fs(directoryPath, true);
            
            FILE* stream = fopen(path.asString().c_str(), "w");
            if (stream == NULL)
                throw FileSystemException("Cannot open file: " + path.asString());
            
            try {
                writeEntities(map->entities(), Model::EntityProperty::EmptyList, 1, stream);
            } catch (...) {
                fclose(stream);
                throw;
            }
            fclose(stream);
        }

        void MapWriter::writeToFileAtPathWithLayers(Model::Map* map, const Path& path, const bool overwrite) {
            if (IO::Disk::fileExists(IO::Disk::fixPath(path)) && !overwrite)
                throw FileSystemException("File already exists: " + path.asString());
            
            // ensure that the directory actually exists or is created if it doesn't
            const Path directoryPath = path.deleteLastComponent();
            WritableDiskFileSystem fs(directoryPath, true);
            
            FILE* stream = fopen(path.asString().c_str(), "w");
            if (stream == NULL)
                throw FileSystemException("Cannot open file: " + path.asString());
            
            try {
                size_t lineNumber = 1;
                
                const Model::LayerList& layers = map->layers();
                assert(!layers.empty());
                
                Model::Entity* worldspawn = map->worldspawn();
                if (worldspawn != NULL)
                    lineNumber += writeDefaultLayer(worldspawn, layers[0], lineNumber, stream);
                for (size_t i = 1; i < layers.size(); ++i)
                    lineNumber += writeLayer(layers[i], lineNumber, stream);
            } catch (...) {
                fclose(stream);
                throw;
            }
            fclose(stream);
        }

        size_t MapWriter::writeDefaultLayer(Model::Entity* worldspawn, const Model::Layer* layer, const size_t lineNumber, FILE* stream) {
            const Model::EntityList& entities = layer->entities();
            const Model::BrushList& worldBrushes = layer->worldBrushes();
            
            size_t lineCount = 0;
            lineCount += writeEntity(worldspawn, Model::EntityProperty::EmptyList, worldBrushes, lineNumber + lineCount, stream);
            lineCount += writeEntities(entities, Model::EntityProperty::EmptyList, lineNumber + lineCount, stream);
            return lineCount;
        }
        
        size_t MapWriter::writeLayer(const Model::Layer* layer, const size_t lineNumber, FILE* stream) {
            const Model::EntityList& entities = layer->entities();
            const Model::BrushList& worldBrushes = layer->worldBrushes();
            
            size_t lineCount = 0;
            lineCount += writeEntityOpen(stream);
            lineCount += writeKeyValuePair(Model::PropertyKeys::Classname, Model::PropertyValues::LayerClassname, stream);
            lineCount += writeKeyValuePair(Model::PropertyKeys::GroupType, Model::PropertyValues::GroupTypeLayer, stream);
            lineCount += writeKeyValuePair(Model::PropertyKeys::LayerName, layer->name(), stream);
            lineCount += writeBrushes(worldBrushes, lineNumber + lineCount, stream);
            lineCount += writeEntityClose(stream);
            
            Model::EntityProperty::List additionalProperties;
            additionalProperties.push_back(Model::EntityProperty(Model::PropertyKeys::Layer, layer->name(), NULL));
            
            lineCount += writeEntities(entities, additionalProperties, lineNumber + lineCount, stream);
            return lineCount;
        }

        size_t MapWriter::writeEntities(const Model::EntityList& entities, const Model::EntityProperty::List& additionalProperties, size_t lineNumber, FILE* stream) {
            Model::EntityList::const_iterator it, end;
            size_t lineCount = 0;
            for (it = entities.begin(), end = entities.end(); it != end; ++it) {
                Model::Entity* entity = *it;
                lineCount += writeEntity(entity, additionalProperties, entity->brushes(), lineNumber + lineCount, stream);
            }
            return lineCount;
        }

        size_t MapWriter::writeEntity(Model::Entity* entity, const Model::EntityProperty::List& additionalProperties, const Model::BrushList& brushes, const size_t lineNumber, FILE* stream) {
            size_t lineCount = 0;
            lineCount += writeEntityHeader(entity, additionalProperties, stream);
            lineCount += writeBrushes(brushes, lineNumber + lineCount, stream);
            lineCount += writeEntityFooter(stream);
            entity->setFilePosition(lineNumber, lineCount);
            return lineCount;
        }

        size_t MapWriter::writeEntityHeader(Model::Entity* entity, const Model::EntityProperty::List& additionalProperties, FILE* stream) {
            size_t lineCount = writeEntityOpen(stream);
            lineCount += writeExtraProperties(entity, stream);
            lineCount += writeEntityProperties(entity->properties(), stream);
            lineCount += writeEntityProperties(additionalProperties, stream);
            return lineCount;
        }

        size_t MapWriter::writeEntityOpen(FILE* stream) {
            std::fprintf(stream, "{\n");
            return 1;
        }

        size_t MapWriter::writeEntityProperties(const Model::EntityProperty::List& properties, FILE* stream) {
            size_t lineCount = 0;
            Model::EntityProperty::List::const_iterator it, end;
            for (it = properties.begin(), end = properties.end(); it != end; ++it)
                lineCount += writeEntityProperty(*it, stream);
            return lineCount;
        }

        size_t MapWriter::writeEntityProperty(const Model::EntityProperty& property, FILE* stream) {
            return writeKeyValuePair(property.key(), property.value(), stream);
        }

        size_t MapWriter::writeKeyValuePair(const String& key, const String& value, FILE* stream) {
            std::fprintf(stream, "\"%s\" \"%s\"\n", key.c_str(), value.c_str());
            return 1;
        }

        size_t MapWriter::writeEntityFooter(FILE* stream) {
            return writeEntityClose(stream);
        }

        size_t MapWriter::writeEntityClose(FILE* stream) {
            std::fprintf(stream, "}\n");
            return 1;
        }

        size_t MapWriter::writeBrushes(const Model::BrushList& brushes, const size_t lineNumber, FILE* stream) {
            size_t lineCount = 0;
            Model::BrushList::const_iterator it, end;
            for (it = brushes.begin(), end = brushes.end(); it != end; ++it)
                lineCount += writeBrush(*it, lineNumber + lineCount, stream);
            return lineCount;
        }
        
        size_t MapWriter::writeBrush(Model::Brush* brush, const size_t lineNumber, FILE* stream) {
            size_t lineCount = 0;
            std::fprintf(stream, "{\n"); ++lineCount;
            
            const Model::BrushFaceList& faces = brush->faces();
            Model::BrushFaceList::const_iterator it, end;
            for (it = faces.begin(), end = faces.end(); it != end; ++it)
                lineCount += writeFace(*it, lineNumber + lineCount, stream);
            
            std::fprintf(stream, "}\n"); ++lineCount;
            
            brush->setFilePosition(lineNumber, lineCount);
            return lineCount;
        }
        
        size_t MapWriter::writeExtraProperties(const Model::Object* object, FILE* stream) {
            const Model::IssueType hiddenIssues = object->hiddenIssues();
            if (hiddenIssues == 0)
                return 0;
            std::fprintf(stream, "/// hideIssues %i\n", hiddenIssues);
            return 1;
        }

        void MapWriter::writeEntity(const Model::Entity* entity, const Model::BrushList& brushes, std::ostream& stream) {
            writeEntityHeader(entity, stream);
            Model::BrushList::const_iterator it, end;
            for (it = brushes.begin(), end = brushes.end(); it != end; ++it)
                writeBrush(*it, stream);
            writeEntityFooter(stream);
        }

        void MapWriter::writeEntityHeader(const Model::Entity* entity, std::ostream& stream) {
            stream << "{\n";
            writeExtraProperties(entity, stream);
            
            const Model::EntityProperty::List& properties = entity->properties();
            Model::EntityProperty::List::const_iterator it, end;
            for (it = properties.begin(), end = properties.end(); it != end; ++it)
                writeEntityProperty(*it, stream);
        }
        
        void MapWriter::writeEntityProperty(const Model::EntityProperty& property, std::ostream& stream) {
            writeKeyValuePair(property.key(), property.value(), stream);
        }

        void MapWriter::writeKeyValuePair(const String& key, const String& value, std::ostream& stream) {
            stream << "\"" << key << "\" \"" << value << "\"" << "\n";
        }

        void MapWriter::writeEntityFooter(std::ostream& stream) {
            stream << "}\n";
        }

        void MapWriter::writeBrush(const Model::Brush* brush, std::ostream& stream) {
            stream << "{\n";
            const Model::BrushFaceList& faces = brush->faces();
            Model::BrushFaceList::const_iterator it, end;
            for (it = faces.begin(), end = faces.end(); it != end; ++it)
                writeFace(*it, stream);
            stream << "}\n";
        }
        
        void MapWriter::writeExtraProperties(const Model::Object* object, std::ostream& stream) {
            const Model::IssueType hiddenIssues = object->hiddenIssues();
            if (hiddenIssues != 0)
                stream << "/// hideIssues " << hiddenIssues << "\n";
        }
    }
}
