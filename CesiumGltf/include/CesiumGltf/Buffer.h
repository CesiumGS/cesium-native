#pragma once

#include "CesiumGltf/BufferCesium.h"
#include "CesiumGltf/BufferSpec.h"

namespace CesiumGltf {
    /** @copydoc BufferSpec */
    struct Buffer : public BufferSpec {
        /**
         * @brief Holds properties that are specific to the glTF loader rather than part of the glTF spec.
         */
        BufferCesium cesium;
    };
}
