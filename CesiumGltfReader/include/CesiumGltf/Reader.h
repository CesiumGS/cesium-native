#pragma once

#include "CesiumGltf/Model.h"
#include <gsl/span>

namespace CesiumGltf {
    struct ModelReaderResult {
        Model model;
        std::string errors;
        std::string warnings;
    };

    ModelReaderResult readModel(const gsl::span<const uint8_t>& data);
}
