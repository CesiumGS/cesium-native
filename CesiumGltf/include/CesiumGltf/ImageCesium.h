// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Library.h"
#include <cstdint>
#include <vector>

namespace CesiumGltf {
    /**
     * @brief Holds {@link Image} properties that are specific to the glTF loader rather than part of the glTF spec.
     */
    struct CESIUMGLTF_API ImageCesium final {
        /**
         * @brief The width of the image in pixels.
         */
        int32_t width;

        /**
         * @brief The height of the image in pixels.
         */
        int32_t height;

        /**
         * @brief The number of channels per pixel.
         */
        int32_t channels;

        /**
         * @brief The number of bytes per channel.
         */
        int32_t bytesPerChannel;

        /**
         * @brief The raw pixel data.
         * 
         * The pixel data is consistent with the [stb](https://github.com/nothings/stb) image library.
         *
         * For a correctly-formed image, the size of the array will be
         * `width * height * channels * bytesPerChannel` bytes. There is no
         * padding between rows or columns of the image, regardless of format.
         * 
         * The channels and their meaning are as follows:
         * 
         * | Number of Channels | Channel Order and Meaning |
         * |--------------------|---------------------------|
         * | 1                  | grey                      |
         * | 2                  | grey, alpha               |
         * | 3                  | red, green, blue          |
         * | 4                  | red, green, blue, alpha   |
         */
        std::vector<uint8_t> pixelData;
    };
}
