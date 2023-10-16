#pragma once

#include "CesiumGltf/PropertyType.h"

#include <gsl/span>

#include <cstddef>

namespace CesiumGltf {
size_t getOffsetFromOffsetsBuffer(
    size_t index,
    const gsl::span<const std::byte>& offsetBuffer,
    PropertyComponentType offsetType) noexcept;
} // namespace CesiumGltf
