#pragma once

#include "ObjectJsonHandler.h"
#include "IntegerJsonHandler.h"
#include <unordered_map>

namespace CesiumGltf {
    template <typename T, typename THandler>
    class DictionaryJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, std::unordered_map<std::string, T>* pDictionary) {
            ObjectJsonHandler::reset(pParent);
            this->_pDictionary = pDictionary;
        }

        virtual JsonHandler* Key(const char* str, size_t /*length*/, bool /*copy*/) override {
            assert(this->_pDictionary);

            return this->property(
                this->_item,
                this->_pDictionary->emplace(str, -1).first->second
            );
        }

    private:
        std::unordered_map<std::string, T>* _pDictionary;
        THandler _item;
    };
}
