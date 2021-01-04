#pragma once

#include "TextureInfoJsonHandler.h"
#include "DoubleJsonHandler.h"

namespace CesiumGltf {
    struct OcclusionTextureInfo;

    class OcclusionTextureInfoJsonHandler : public TextureInfoJsonHandler {
    public:
        void reset(JsonHandler* pParent, OcclusionTextureInfo* pOcclusionTextureInfo);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        OcclusionTextureInfo* _pOcclusionTextureInfo = nullptr;

        DoubleJsonHandler _strength;
    };
}
