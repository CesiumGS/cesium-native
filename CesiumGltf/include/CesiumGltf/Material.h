#pragma once

#include "CesiumGltf/AlphaMode.h"
#include "CesiumGltf/NamedObject.h"
#include "CesiumGltf/NormalTextureInfo.h"
#include "CesiumGltf/OcclusionTextureInfo.h"
#include "CesiumGltf/PbrMetallicRoughness.h"
#include "CesiumGltf/TextureInfo.h"
#include <vector>

namespace CesiumGltf {
    struct Material : public NamedObject {
        PbrMetallicRoughness pbrMetallicRoughness;
        NormalTextureInfo normalTexture;
        OcclusionTextureInfo occlusionTexture;
        TextureInfo emissiveTexture;
        std::vector<double> emissiveFactor;
        AlphaMode alphaMode;
        double alphaCutoff;
        bool doubleSided;
    };
}
