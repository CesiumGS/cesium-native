#pragma once

#include "CesiumGltf/ReaderLibrary.h"
#include "CesiumGltf/Model.h"
#include <gsl/span>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGltf {
    /**
     * @brief The result of reading a glTF model with {@link readModel}.
     */
    struct CESIUMGLTFREADER_API ModelReaderResult {
        /**
         * @brief The read model, or std::nullopt if the model could not be read.
         */
        std::optional<Model> model;

        /**
         * @brief Errors, if any, that occurred during the load process.
         */
        std::vector<std::string> errors;

        /**
         * @brief Warnings, if any, that occurred during the load process.
         */
        std::vector<std::string> warnings;
    };

    /**
     * @brief Options for how to read a glTF.
     */
    struct CESIUMGLTFREADER_API ReadModelOptions {
        /**
         * @brief Whether data URIs should be automatically decoded as part of the load process.
         */
        bool decodeDataUris = true;

        /**
         * @brief Whether data URIs should be cleared after they are successfully decoded.
         * 
         * This reduces the memory usage of the model.
         */
        bool clearDecodedDataUris = true;

        /**
         * @brief Whether embedded images in {@link Model:buffers} should be automatically decoded as part of the load process.
         * 
         * The {@link ImageSpec::mimeType} property is ignored, and instead the [stb_image](https://github.com/nothings/stb) library
         * is used to decode images in `JPG`, `PNG`, `TGA`, `BMP`, `PSD`, `GIF`, `HDR`, or `PIC` format.
         */
        bool decodeEmbeddedImages = true;

        /**
         * @brief Whether geometry compressed using the `KHR_draco_mesh_compression` extension should be automatically decoded
         *        as part of the load process.
         */
        bool decodeDraco = true;
    };

    /**
     * @brief Reads a glTF or binary glTF (GLB) from a buffer.
     * 
     * @param data The buffer from which to read the glTF.
     * @param options Options for how to read the glTF.
     * @return The result of reading the glTF.
     */
    CESIUMGLTFREADER_API ModelReaderResult readModel(const gsl::span<const uint8_t>& data, const ReadModelOptions& options = ReadModelOptions());

    /**
     * @brief The result of reading an image with {@link readImage}.
     */
    struct CESIUMGLTFREADER_API ImageReaderResult {
        std::optional<ImageCesium> image;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    /**
     * @brief Reads an image from a buffer.
     * 
     * The [stb_image](https://github.com/nothings/stb) library is used to decode images in `JPG`, `PNG`, `TGA`,
     * `BMP`, `PSD`, `GIF`, `HDR`, or `PIC` format.
     * 
     * @param data The buffer from which to read the image.
     * @return The result of reading the image.
     */
    CESIUMGLTFREADER_API ImageReaderResult readImage(const gsl::span<const uint8_t>& data);
}
