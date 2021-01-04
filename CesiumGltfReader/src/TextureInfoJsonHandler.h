#pragma once

#include "ExtensibleObjectJsonHandler.h"
#include "IntegerJsonHandler.h"

namespace CesiumGltf {
    struct TextureInfo;

    class TextureInfoJsonHandler : public ExtensibleObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, TextureInfo* pTextureInfo);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        TextureInfo* _pTextureInfo = nullptr;

        IntegerJsonHandler<int32_t> _index;
        IntegerJsonHandler<int32_t> _texCoord;
    };
}
