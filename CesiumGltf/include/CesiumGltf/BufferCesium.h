#pragma once

#include <vector>

namespace CesiumGltf {
    struct BufferCesium final {
        /**
         * @brief The buffer's data.
         */
        std::vector<uint8_t> data;
    };
}
