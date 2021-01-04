#pragma once

#include "JsonHandler.h"
#include "IgnoreValueJsonHandler.h"

namespace CesiumGltf {
    class ObjectJsonHandler : public JsonHandler {
    public:
        virtual JsonHandler* StartObject() override {
            return this;
        }

        virtual JsonHandler* EndObject(size_t /*memberCount*/) override {
            return this->parent();
        }

        JsonHandler* ignore() {
            this->_ignoreHandler.reset(this);
            return &this->_ignoreHandler;
        }

    private:
        IgnoreValueJsonHandler _ignoreHandler;
    };
}
