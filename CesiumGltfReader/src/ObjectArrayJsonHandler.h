#pragma once

#include "JsonHandler.h"
#include <cassert>
#include <vector>

namespace CesiumGltf {
    template <typename T, typename THandler>
    class ObjectArrayJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, std::vector<T>* pArray) {
            this->_pParent = pParent;
            this->_pArray = pArray;
            this->_arrayIsOpen = false;
        }

        virtual JsonHandler* StartArray() override {
            if (this->_arrayIsOpen) {
                return nullptr;
            }

            this->_arrayIsOpen = true;
            return this;
        }

        virtual JsonHandler* EndArray(size_t) override {
            return this->_pParent;
        }

        virtual JsonHandler* StartObject() override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            T& o = this->_pArray->emplace_back();
            this->_objectHandler.reset(this, &o);
            return this->_objectHandler.StartObject();
        }

    private:
        JsonHandler* _pParent = nullptr;
        std::vector<T>* _pArray = nullptr;
        bool _arrayIsOpen = false;
        THandler _objectHandler;
    };
}
