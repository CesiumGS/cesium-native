#pragma once

#include "Library.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace CesiumGeometry {

struct CESIUMGEOMETRY_API ConstantAvailability {
  uint8_t constant;
};

struct CESIUMGEOMETRY_API SubtreeBufferView {
  uint32_t byteOffset;
  uint32_t byteLength;
  uint8_t buffer;
};

typedef std::variant<ConstantAvailability, SubtreeBufferView> AvailabilityView;

struct CESIUMGEOMETRY_API AvailabilitySubtree {
  AvailabilityView tileAvailability;
  AvailabilityView contentAvailability;
  AvailabilityView subtreeAvailability;
  std::vector<std::vector<std::byte>> buffers;
};
} // namespace CesiumGeometry