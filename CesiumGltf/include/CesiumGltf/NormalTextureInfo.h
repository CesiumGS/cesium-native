#pragma once

#include "CesiumGltf/TextureInfo.h"

namespace CesiumGltf {
    struct NormalTextureInfo : public TextureInfo {
        double scale;
    };
}
