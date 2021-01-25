#pragma once

#include "CesiumGltf/ReaderLibrary.h"
#include "CesiumGltf/Model.h"
#include <gsl/span>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGltf {
    struct CESIUMGLTFREADER_API ModelReaderResult {
        std::optional<Model> model;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    struct CESIUMGLTFREADER_API ReadModelOptions {
        bool decodeDataUris = true;
        bool decodeEmbeddedImages = true;
        bool decodeDraco = true;
    };

    CESIUMGLTFREADER_API ModelReaderResult readModel(const gsl::span<const uint8_t>& data, const ReadModelOptions& options = ReadModelOptions());

    struct CESIUMGLTFREADER_API ImageReaderResult {
        std::optional<ImageCesium> image;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    CESIUMGLTFREADER_API ImageReaderResult readImage(const gsl::span<const uint8_t>& data);
}
