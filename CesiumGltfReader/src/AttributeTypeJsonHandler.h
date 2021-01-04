#pragma once

#include "JsonHandler.h"
#include "CesiumGltf/AttributeType.h"

namespace CesiumGltf {
    class AttributeTypeJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, AttributeType* pEnum);
        virtual JsonHandler* String(const char* str, size_t length, bool copy) override;

    private:
        AttributeType* _pEnum = nullptr;
    };
}
