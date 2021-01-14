#pragma once

#include "ObjectJsonHandler.h"
#include "IntegerJsonHandler.h"
#include <unordered_map>

namespace CesiumGltf {
    template <typename T, typename THandler>
    class DictionaryJsonHandler : public ObjectJsonHandler {
    public:
        void reset(IJsonHandler* pParent, std::unordered_map<std::string, T>* pDictionary) {
            ObjectJsonHandler::reset(pParent);
            this->_pDictionary = pDictionary;
        }

        std::unordered_map<std::string, T>* getObject() { return this->_pDictionary; }

        virtual IJsonHandler* Key(const char* str, size_t /*length*/, bool /*copy*/) override {
            assert(this->_pDictionary);

            auto it = this->_pDictionary->emplace(str, T()).first;

            return this->property(
                it->first.c_str(),
                this->_item,
                it->second
            );
        }

    private:
        std::unordered_map<std::string, T>* _pDictionary;
        THandler _item;
    };
}
