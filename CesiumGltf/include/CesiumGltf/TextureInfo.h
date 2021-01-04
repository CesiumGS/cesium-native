#pragma once

#include "CesiumGltf/ExtensibleObject.h"

namespace CesiumGltf {
    struct TextureInfo : public ExtensibleObject {
        int32_t index;
        int32_t texCoord;
    };
}
