#include "CesiumGltf/TPropertyView.h"
#include <cassert>

namespace CesiumGltf {
namespace Impl {
size_t getOffsetFromOffsetBuffer(
    size_t instance,
    const gsl::span<const std::byte>& offsetBuffer,
    PropertyType offsetType) {
  switch (offsetType) {
  case PropertyType::Uint8: {
    uint8_t offset = *reinterpret_cast<const uint8_t*>(
        offsetBuffer.data() + instance * sizeof(uint8_t));
    return static_cast<size_t>(offset);
  }
  case PropertyType::Uint16: {
    uint16_t offset = *reinterpret_cast<const uint16_t*>(
        offsetBuffer.data() + instance * sizeof(uint16_t));
    return static_cast<size_t>(offset);
  }
  case PropertyType::Uint32: {
    uint32_t offset = *reinterpret_cast<const uint32_t*>(
        offsetBuffer.data() + instance * sizeof(uint32_t));
    return static_cast<size_t>(offset);
  }
  case PropertyType::Uint64: {
    uint64_t offset = *reinterpret_cast<const uint64_t*>(
        offsetBuffer.data() + instance * sizeof(uint64_t));
    return static_cast<size_t>(offset);
  }
  default:
    assert(false && "Offset type has unknown type");
    return 0;
  }
}
} // namespace Impl
} // namespace CesiumGltf