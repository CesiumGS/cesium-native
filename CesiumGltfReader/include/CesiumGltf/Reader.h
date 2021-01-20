#pragma once

#include "CesiumGltf/Model.h"
#include <gsl/span>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGltf {
    struct ModelReaderResult {
        std::optional<Model> model;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    struct ReadModelOptions {
        bool decodeDataUris = true;
        bool decodeEmbeddedImages = true;
        bool decodeDraco = true;
    };

    ModelReaderResult readModel(const gsl::span<const uint8_t>& data, const ReadModelOptions& options = ReadModelOptions());

    struct ImageReaderResult {
        std::optional<ImageCesium> image;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    ImageReaderResult readImage(const gsl::span<const uint8_t>& data);
}
