#include <CesiumGltf/PropertyTexturePropertyView.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyView.h>

#include <cstdint>
#include <span>

namespace CesiumGltf {

// Re-initialize consts here to avoid "undefined reference" errors with GCC /
// Clang.
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidPropertyTexture;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidTexture;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidSampler;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidImage;
const PropertyViewStatusType PropertyTexturePropertyViewStatus::ErrorEmptyImage;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidChannels;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch;

int64_t assembleEnumValue(
    const std::span<uint8_t> bytes,
    PropertyComponentType componentType) noexcept {
  switch (componentType) {
  case PropertyComponentType::Int8:
    return static_cast<int64_t>(assembleScalarValue<int8_t>(bytes));
  case PropertyComponentType::Uint8:
    return static_cast<int64_t>(assembleScalarValue<uint8_t>(bytes));
  case PropertyComponentType::Int16:
    return static_cast<int64_t>(assembleScalarValue<int16_t>(bytes));
  case PropertyComponentType::Uint16:
    return static_cast<int64_t>(assembleScalarValue<uint16_t>(bytes));
  case PropertyComponentType::Int32:
    return static_cast<int64_t>(assembleScalarValue<int32_t>(bytes));
  case PropertyComponentType::Uint32:
    return static_cast<int64_t>(assembleScalarValue<uint32_t>(bytes));
  case PropertyComponentType::Int64:
    return static_cast<int64_t>(assembleScalarValue<int64_t>(bytes));
  case PropertyComponentType::Uint64:
    return static_cast<int64_t>(assembleScalarValue<uint64_t>(bytes));
  default:
    return {};
  }
}

} // namespace CesiumGltf
