#pragma once

#include "CesiumGltf/Library.h"
#include <vector>
#include <cstddef>

namespace CesiumGltf {
    /**
     * @brief Holds {@link Buffer} properties that are specific to the glTF loader rather than part of the glTF spec.
     */
    struct CESIUMGLTF_API BufferCesium final {
        /**
         * @brief The buffer's data.
         */
        std::vector<std::byte> data;
    };
}
