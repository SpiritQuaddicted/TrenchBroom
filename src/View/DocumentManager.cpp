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

#include "DocumentManager.h"

#include "Exceptions.h"

#include <cassert>

namespace TrenchBroom {
    namespace View {
        DocumentManager::DocumentManager(const bool singleDocument) :
        m_singleDocument(singleDocument) {}
        
        DocumentManager::~DocumentManager() {
            closeAllDocuments();
        }

        const DocumentList& DocumentManager::documents() const {
            return m_documents;
        }

        MapDocument::Ptr DocumentManager::newDocument() {
            MapDocument::Ptr document = createOrReuseDocument();
            document->newDocument();
            return document;
        }
        
        MapDocument::Ptr DocumentManager::openDocument(const IO::Path& path) {
            MapDocument::Ptr document = createOrReuseDocument();
            document->openDocument(path);
            return document;
        }
        
        bool DocumentManager::closeDocument(MapDocument::Ptr document) {
            DocumentList::iterator it = std::find(m_documents.begin(), m_documents.end(), document);
            if (it == m_documents.end())
                throw DocumentManagerException("Unknown document");
            
            if (document->closeDocument()) {
                m_documents.erase(it);
                return true;
            }
            return false;
        }
        
        bool DocumentManager::closeAllDocuments() {
            DocumentList::iterator it = m_documents.begin();
            while (it != m_documents.end()) {
                MapDocument::Ptr document = *it;
                if (document->closeDocument())
                    it = m_documents.erase(it);
                else
                    ++it;
            }
            return m_documents.empty();
        }

        MapDocument::Ptr DocumentManager::createOrReuseDocument() {
            assert(!m_singleDocument || m_documents.size() <= 1);
            if (m_singleDocument && m_documents.size() == 1) {
                return m_documents.front();
            } else {
                MapDocument::Ptr document = MapDocument::newMapDocument();
                m_documents.push_back(document);
                return document;
            }
        }
    }
}