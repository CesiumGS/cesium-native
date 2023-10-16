#include "CesiumGltf/PropertyArrayView.h"

namespace CesiumGltf {

PropertyArrayView<bool>::PropertyArrayView() noexcept
    : _values{}, _bitOffset{0}, _size{0} {}

PropertyArrayView<bool>::PropertyArrayView(
    const gsl::span<const std::byte>& buffer,
    int64_t bitOffset,
    int64_t size) noexcept
    : _values{buffer}, _bitOffset{bitOffset}, _size{size} {}

bool PropertyArrayView<bool>::operator[](int64_t index) const noexcept {
  index += _bitOffset;
  const uint64_t byteIndex = static_cast<uint64_t>(index / 8);
  const uint8_t bitIndex = static_cast<uint8_t>(index % 8);
  const uint8_t bitValue =
      (static_cast<uint8_t>(_values[byteIndex]) >> bitIndex) & 1;
  return bitValue == 1;
}

int64_t PropertyArrayView<bool>::size() const noexcept { return _size; }

bool PropertyArrayView<bool>::operator==(
    const PropertyArrayView<bool>& other) const noexcept {
  if (this->size() != other.size()) {
    return false;
  }

  for (int64_t i = 0; i < size(); i++) {
    if ((*this)[i] != other[i]) {
      return false;
    }
  }

  return true;
}

bool PropertyArrayView<bool>::operator!=(
    const PropertyArrayView<bool>& other) const noexcept {
  return !operator==(other);
}

PropertyArrayView<bool>::~PropertyArrayView() noexcept = default;
PropertyArrayView<bool>::PropertyArrayView(
    const PropertyArrayView<bool>& rhs) noexcept = default;
PropertyArrayView<bool>::PropertyArrayView(
    PropertyArrayView<bool>&& rhs) noexcept = default;
PropertyArrayView<bool>& PropertyArrayView<bool>::operator=(
    const PropertyArrayView<bool>& rhs) noexcept = default;
PropertyArrayView<bool>& PropertyArrayView<bool>::operator=(
    PropertyArrayView<bool>&& rhs) noexcept = default;

PropertyArrayView<std::string_view>::PropertyArrayView() noexcept
    : _values{},
      _stringOffsets{},
      _stringOffsetType{PropertyComponentType::None},
      _size{0} {}

PropertyArrayView<std::string_view>::PropertyArrayView(
    const gsl::span<const std::byte>& values,
    const gsl::span<const std::byte>& stringOffsets,
    PropertyComponentType stringOffsetType,
    int64_t size) noexcept
    : _values{values},
      _stringOffsets{stringOffsets},
      _stringOffsetType{stringOffsetType},
      _size{size} {}

std::string_view
PropertyArrayView<std::string_view>::operator[](int64_t index) const noexcept {
  const size_t currentOffset = getOffsetFromOffsetsBuffer(
      static_cast<size_t>(index),
      _stringOffsets,
      _stringOffsetType);
  const size_t nextOffset = getOffsetFromOffsetsBuffer(
      static_cast<size_t>(index + 1),
      _stringOffsets,
      _stringOffsetType);
  return std::string_view(
      reinterpret_cast<const char*>(_values.data() + currentOffset),
      (nextOffset - currentOffset));
}

int64_t PropertyArrayView<std::string_view>::size() const noexcept {
  return _size;
}

bool PropertyArrayView<std::string_view>::operator==(
    const PropertyArrayView<std::string_view>& other) const noexcept {
  if (this->size() != other.size()) {
    return false;
  }

  for (int64_t i = 0; i < size(); i++) {
    if ((*this)[i] != other[i]) {
      return false;
    }
  }

  return true;
}

bool PropertyArrayView<std::string_view>::operator!=(
    const PropertyArrayView<std::string_view>& other) const noexcept {
  return !operator==(other);
}

PropertyArrayView<std::string_view>::~PropertyArrayView() noexcept = default;
PropertyArrayView<std::string_view>::PropertyArrayView(
    const PropertyArrayView<std::string_view>& rhs) noexcept = default;
PropertyArrayView<std::string_view>::PropertyArrayView(
    PropertyArrayView<std::string_view>&& rhs) noexcept = default;
PropertyArrayView<std::string_view>&
PropertyArrayView<std::string_view>::operator=(
    const PropertyArrayView<std::string_view>& rhs) noexcept = default;
PropertyArrayView<std::string_view>&
PropertyArrayView<std::string_view>::operator=(
    PropertyArrayView<std::string_view>&& rhs) noexcept = default;

} // namespace CesiumGltf
