#pragma once

#include "DoubleArrayJsonHandler.h"
#include "DoubleJsonHandler.h"
#include "ExtensibleObjectJsonHandler.h"
#include "TextureInfoJsonHandler.h"

namespace CesiumGltf {
    struct PbrMetallicRoughness;

    class PbrMetallicRoughnessJsonHandler : public ExtensibleObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, PbrMetallicRoughness* pPbr);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        PbrMetallicRoughness* _pPbr = nullptr;

        DoubleArrayJsonHandler _baseColorFactor;
        TextureInfoJsonHandler _baseColorTexture;
        DoubleJsonHandler _metallicFactor;
        DoubleJsonHandler _roughnessFactor;
        TextureInfoJsonHandler _metallicRoughnessTexture;
    };
}
