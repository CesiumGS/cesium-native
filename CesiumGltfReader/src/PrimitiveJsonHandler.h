#pragma once

#include "CesiumGltf/PrimitiveMode.h"
#include "ObjectJsonHandler.h"
#include "IntegerJsonHandler.h"

namespace CesiumGltf {
    struct Primitive;

    class PrimitiveJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Primitive* pPrimitive);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        Primitive* _pPrimitive = nullptr;
        // std::unordered_map<std::string, size_t> attributes;
        IntegerJsonHandler<int32_t> _indices;
        IntegerJsonHandler<int32_t> _material;
        IntegerJsonHandler<PrimitiveMode> _mode;
        // std::vector<std::unordered_map<std::string, size_t>> targets;
    };
}
