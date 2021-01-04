#pragma once

#include "CesiumGltf/TextureInfo.h"

namespace CesiumGltf {
    struct OcclusionTextureInfo : public TextureInfo {
        double strength;
    };
}
