#pragma once

#include "JsonHandler.h"
#include "IgnoreValueJsonHandler.h"

namespace CesiumGltf {
    class ObjectJsonHandler : public JsonHandler {
    public:
        virtual JsonHandler* StartObject() override final;
        virtual JsonHandler* EndObject(size_t memberCount) override final;

    protected:
        virtual JsonHandler* StartSubObject();
        virtual JsonHandler* EndSubObject(size_t memberCount);

        JsonHandler* ignore();

    private:
        IgnoreValueJsonHandler _ignoreHandler;
        int32_t _depth = 0;
    };
}
