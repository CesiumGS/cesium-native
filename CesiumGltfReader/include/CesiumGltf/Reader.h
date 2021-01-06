#pragma once

#include "CesiumGltf/Model.h"
#include <gsl/span>
#include <optional>

namespace CesiumGltf {
    struct ModelReaderResult {
        std::optional<Model> model;
        std::string errors;
        std::string warnings;
    };

    ModelReaderResult readModel(const gsl::span<const uint8_t>& data);
}
