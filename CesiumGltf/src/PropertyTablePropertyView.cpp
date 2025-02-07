#include <CesiumGltf/PropertyTablePropertyView.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyView.h>

#include <cstdint>

namespace CesiumGltf {
// Re-initialize consts here to avoid "undefined reference" errors with GCC /
// Clang.
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidValueBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidValueBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorBufferViewOutOfBounds;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize;
const PropertyViewStatusType PropertyTablePropertyViewStatus::
    ErrorBufferViewSizeDoesNotMatchPropertyTableCount;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetType;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayOffsetsNotSorted;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorStringOffsetsNotSorted;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayOffsetOutOfBounds;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorStringOffsetOutOfBounds;

int64_t getOffsetTypeSize(PropertyComponentType offsetType) noexcept {
  switch (offsetType) {
  case PropertyComponentType::Uint8:
    return sizeof(uint8_t);
  case PropertyComponentType::Uint16:
    return sizeof(uint16_t);
  case PropertyComponentType::Uint32:
    return sizeof(uint32_t);
  case PropertyComponentType::Uint64:
    return sizeof(uint64_t);
  default:
    return 0;
  }
}
} // namespace CesiumGltf
