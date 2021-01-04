#pragma once

#include "TextureInfoJsonHandler.h"
#include "DoubleJsonHandler.h"

namespace CesiumGltf {
    struct NormalTextureInfo;

    class NormalTextureInfoJsonHandler : public TextureInfoJsonHandler {
    public:
        void reset(JsonHandler* pParent, NormalTextureInfo* pNormalTextureInfo);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        NormalTextureInfo* _pNormalTextureInfo = nullptr;

        DoubleJsonHandler _scale;
    };
}
