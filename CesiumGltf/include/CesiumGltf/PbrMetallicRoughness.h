#pragma once

#include "CesiumGltf/ExtensibleObject.h"
#include "CesiumGltf/TextureInfo.h"
#include <vector>

namespace CesiumGltf {
    struct PbrMetallicRoughness : public ExtensibleObject {
        std::vector<double> baseColorFactor;
        TextureInfo baseColorTexture;
        double metallicFactor;
        double roughnessFactor;
        TextureInfo metallicRoughnessTexture;
    };
}
