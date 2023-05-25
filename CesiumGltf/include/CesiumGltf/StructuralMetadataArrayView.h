#pragma once

#include "CesiumGltf/StructuralMetadataPropertyType.h"
#include "getOffsetFromOffsetsBuffer.h"

#include <CesiumUtility/SpanHelper.h>

#include <gsl/span>

#include <cassert>
#include <cstddef>

namespace CesiumGltf {
namespace StructuralMetadata {

/**
 * @brief A view on an array element of a
 * {@link ExtensionExtStructuralMetadataPropertyTableProperty}.
 *
 * Provides utility to retrieve the data stored in the array of
 * elements via the array index operator.
 */
template <typename ElementType> class MetadataArrayView {
public:
  MetadataArrayView() : _values{} {}

  MetadataArrayView(const gsl::span<const std::byte>& buffer) noexcept
      : _values{CesiumUtility::reintepretCastSpan<const ElementType>(buffer)} {}

  const ElementType& operator[](int64_t index) const noexcept {
    return _values[index];
  }

  int64_t size() const noexcept { return static_cast<int64_t>(_values.size()); }

private:
  const gsl::span<const ElementType> _values;
};

template <> class MetadataArrayView<bool> {
public:
  MetadataArrayView() : _values{}, _bitOffset{0}, _size{0} {}

  MetadataArrayView(
      const gsl::span<const std::byte>& buffer,
      int64_t bitOffset,
      int64_t size) noexcept
      : _values{buffer}, _bitOffset{bitOffset}, _size{size} {}

  bool operator[](int64_t index) const noexcept {
    index += _bitOffset;
    const int64_t byteIndex = index / 8;
    const int64_t bitIndex = index % 8;
    const int bitValue = static_cast<int>(_values[byteIndex] >> bitIndex) & 1;
    return bitValue == 1;
  }

  int64_t size() const noexcept { return _size; }

private:
  gsl::span<const std::byte> _values;
  int64_t _bitOffset;
  int64_t _size;
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
    const size_t currentOffset =
        getOffsetFromOffsetsBuffer(index, _stringOffsets, _stringOffsetType);
    const size_t nextOffset = getOffsetFromOffsetsBuffer(
        index + 1,
        _stringOffsets,
        _stringOffsetType);
    return std::string_view(
        reinterpret_cast<const char*>(_values.data() + currentOffset),
        (nextOffset - currentOffset));
  }

  int64_t size() const noexcept { return _size; }

private:
  gsl::span<const std::byte> _values;
  gsl::span<const std::byte> _stringOffsets;
  StructuralMetadata::PropertyComponentType _stringOffsetType;
  int64_t _size;
};

} // namespace StructuralMetadata
} // namespace CesiumGltf
