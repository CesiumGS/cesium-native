#pragma once

#include "CesiumGltf/StructuralMetadataPropertyType.h"

#include <CesiumUtility/SpanHelper.h>

#include <gsl/span>

#include <cassert>
#include <cstddef>

/**
 * @brief A view on an array element of a
 * ExtensionExtStructuralMetadataPropertyTableProperty.
 *
 * Provides utility to retrieve the data stored in the array of
 * elements via the array index operator.
 */
namespace CesiumGltf {
namespace StructuralMetadata {

template <typename ElementType> class MetadataArrayView {
public:
  MetadataArrayView() : _values{} {}

  MetadataArrayView(const gsl::span<const std::byte>& buffer) noexcept
      : _values{
            CesiumUtility::reintepretCastSpan<const ElementType>(buffer)} {}

  const ElementType& operator[](int64_t index) const noexcept {
    return _values[index];
  }

  int64_t size() const noexcept {
    return static_cast<int64_t>(_values.size());
  }

private:
  gsl::span<const ElementType> _values;
};

template <> class MetadataArrayView<bool> {
public:
  MetadataArrayView()
      : _values{}, _bitOffset{0}, _instanceCount{0} {}

  MetadataArrayView(
      const gsl::span<const std::byte>& buffer,
      int64_t bitOffset,
      int64_t instanceCount) noexcept
      : _values{buffer},
        _bitOffset{bitOffset},
        _instanceCount{instanceCount} {}

  bool operator[](int64_t index) const noexcept {
    index += _bitOffset;
    const int64_t byteIndex = index / 8;
    const int64_t bitIndex = index % 8;
    const int bitValue =
        static_cast<int>(_values[byteIndex] >> bitIndex) & 1;
    return bitValue == 1;
  }

  int64_t size() const noexcept { return _instanceCount; }

private:
  gsl::span<const std::byte> _values;
  int64_t _bitOffset;
  int64_t _instanceCount;
};

template <> class MetadataArrayView<std::string_view> {
public:
  MetadataArrayView()
      : _values{}, _stringOffsets{}, _stringOffsetType{}, _size{0} {}

  MetadataArrayView(
      const gsl::span<const std::byte>& values,
      const gsl::span<const std::byte>& stringOffsets,
      StructuralMetadata::PropertyComponentType stringOffsetType,
      int64_t size) noexcept
      : _values{values},
        _stringOffsets{stringOffsets},
        _stringOffsetType{stringOffsetType},
        _size{size} {}

  std::string_view operator[](int64_t index) const noexcept {
    const size_t currentOffset = getOffsetFromStringOffsetsBuffer(
        index,
        _stringOffsets,
        _stringOffsetType);
    const size_t nextOffset = getOffsetFromStringOffsetsBuffer(
        index + 1,
        _stringOffsets,
        _stringOffsetType);
    return std::string_view(
        reinterpret_cast<const char*>(_values.data() + currentOffset),
        (nextOffset - currentOffset));
  }

  int64_t size() const noexcept { return _size; }

private:
  static size_t getOffsetFromStringOffsetsBuffer(
      size_t instance,
      const gsl::span<const std::byte>& stringOffsetBuffer,
      StructuralMetadata::PropertyComponentType offsetType) noexcept {
    switch (offsetType) {
    case StructuralMetadata::PropertyComponentType::Uint8: {
      assert(instance < stringOffsetBuffer.size() / sizeof(uint8_t));
      const uint8_t offset = *reinterpret_cast<const uint8_t*>(
          stringOffsetBuffer.data() + instance * sizeof(uint8_t));
      return static_cast<size_t>(offset);
    }
    case StructuralMetadata::PropertyComponentType::Uint16: {
      assert(instance < stringOffsetBuffer.size() / sizeof(uint16_t));
      const uint16_t offset = *reinterpret_cast<const uint16_t*>(
          stringOffsetBuffer.data() + instance * sizeof(uint16_t));
      return static_cast<size_t>(offset);
    }
    case StructuralMetadata::PropertyComponentType::Uint32: {
      assert(instance < stringOffsetBuffer.size() / sizeof(uint32_t));
      const uint32_t offset = *reinterpret_cast<const uint32_t*>(
          stringOffsetBuffer.data() + instance * sizeof(uint32_t));
      return static_cast<size_t>(offset);
    }
    case StructuralMetadata::PropertyComponentType::Uint64: {
      assert(instance < stringOffsetBuffer.size() / sizeof(uint64_t));
      const uint64_t offset = *reinterpret_cast<const uint64_t*>(
          stringOffsetBuffer.data() + instance * sizeof(uint64_t));
      return static_cast<size_t>(offset);
    }
    default:
      assert(false && "Offset type has unknown type");
      return 0;
    }
  }
  gsl::span<const std::byte> _values;
  gsl::span<const std::byte> _stringOffsets;
  StructuralMetadata::PropertyComponentType _stringOffsetType;
  int64_t _size;
};

} // namespace StructuralMetadata
} // namespace CesiumGltf
