#pragma once

#include "AlphaModeJsonHandler.h"
#include "BoolJsonHandler.h"
#include "DoubleArrayJsonHandler.h"
#include "DoubleJsonHandler.h"
#include "NamedObjectJsonHandler.h"
#include "NormalTextureInfoJsonHandler.h"
#include "OcclusionTextureInfoJsonHandler.h"
#include "PbrMetallicRoughnessJsonHandler.h"
#include "TextureInfoJsonHandler.h"
#include <string>

namespace CesiumGltf {
    struct Material;

    class MaterialJsonHandler : public NamedObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Material* pMaterial);

        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        Material* _pMaterial = nullptr;

        PbrMetallicRoughnessJsonHandler _pbrMetallicRoughness;
        NormalTextureInfoJsonHandler normalTexture;
        OcclusionTextureInfoJsonHandler occlusionTexture;
        TextureInfoJsonHandler _emissiveTexture;
        DoubleArrayJsonHandler _emissiveFactor;
        AlphaModeJsonHandler _alphaMode;
        DoubleJsonHandler _alphaCutoff;
        BoolJsonHandler _doubleSided;
    };
}
