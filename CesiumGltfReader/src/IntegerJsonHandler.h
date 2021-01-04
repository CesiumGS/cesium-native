#pragma once

#include "JsonHandler.h"
#include <cassert>

namespace CesiumGltf {
    template <typename T>
    class IntegerJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, T* pInteger) {
            JsonHandler::reset(pParent);
            this->_pInteger = pInteger;
        }

        virtual JsonHandler* Int(int i) override {
            assert(this->_pInteger);
            *this->_pInteger = static_cast<T>(i);
            return this->parent();
        }
        virtual JsonHandler* Uint(unsigned i) override {
            assert(this->_pInteger);
            *this->_pInteger = static_cast<T>(i);
            return this->parent();
        }
        virtual JsonHandler* Int64(int64_t i) override {
            assert(this->_pInteger);
            *this->_pInteger = static_cast<T>(i);
            return this->parent();
        }
        virtual JsonHandler* Uint64(uint64_t i) override {
            assert(this->_pInteger);
            *this->_pInteger = static_cast<T>(i);
            return this->parent();
        }

    private:
        T* _pInteger = nullptr;
    };
}
