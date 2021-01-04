#pragma once

#include "ObjectJsonHandler.h"
#include "IntegerJsonHandler.h"
#include <unordered_map>

namespace CesiumGltf {
    class AttributeJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, std::unordered_map<std::string, int32_t>* pAttributes);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        std::unordered_map<std::string, int32_t>* _pAttributes;
        IntegerJsonHandler<int32_t> _index;
    };
}