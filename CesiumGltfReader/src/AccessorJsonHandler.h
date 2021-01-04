#pragma once

#include "AttributeTypeJsonHandler.h"
#include "BoolJsonHandler.h"
#include "CesiumGltf/Accessor.h"
#include "DoubleArrayJsonHandler.h"
#include "IntegerJsonHandler.h"
#include "ObjectJsonHandler.h"
#include <string>

namespace CesiumGltf {
    class AccessorJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Accessor* pAccessor);

        virtual JsonHandler* Key(const char* str, size_t /*length*/, bool /*copy*/) override;

    private:
        Accessor* _pAccessor = nullptr;
        IntegerJsonHandler<int32_t> _bufferView;
        IntegerJsonHandler<int64_t> _byteOffset;
        IntegerJsonHandler<ComponentType> _componentType;
        BoolJsonHandler _normalized;
        IntegerJsonHandler<int64_t> _count;
        AttributeTypeJsonHandler _type;
        DoubleArrayJsonHandler _max;
        DoubleArrayJsonHandler _min;
    };
}
