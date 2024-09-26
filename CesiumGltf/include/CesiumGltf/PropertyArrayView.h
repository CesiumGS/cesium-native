#pragma once

#include "CesiumGltf/PropertyType.h"
#include "getOffsetFromOffsetsBuffer.h"

#include <CesiumUtility/SpanHelper.h>

#include <gsl/span>

#include <cstddef>
#include <cstring>
#include <variant>
#include <vector>

namespace CesiumGltf {

/**
 * @brief A view on an array element of a {@link PropertyTableProperty}
 * or {@link PropertyTextureProperty}.
 *
 * Provides utility to retrieve the data stored in the array of
 * elements via the array index operator.
 */
template <typename ElementType> class PropertyArrayView {
public:
  /**
   * @brief Constructs an empty array view.
   */
  PropertyArrayView() : _values{} {}

  /**
   * @brief Constructs an array view from a buffer.
   *
   * @param buffer The buffer containing the values.
   */
  PropertyArrayView(const gsl::span<const std::byte>& buffer) noexcept
      : _values{CesiumUtility::reintepretCastSpan<const ElementType>(buffer)} {}

  const ElementType& operator[](int64_t index) const noexcept {
    return this->_values[index];
  }

  int64_t size() const noexcept { return this->_values.size(); }

  auto begin() { return this->_values.begin(); }
  auto end() { return this->_values.end(); }
  auto begin() const { return this->_values.begin(); }
  auto end() const { return this->_values.end(); }

private:
  gsl::span<const ElementType> _values;
};

/**
 * @brief A copy of an array element of a {@link PropertyTableProperty} or
 * {@link PropertyTextureProperty}.
 *
 * Whereas {@link PropertyArrayView} is a pointer to data stored in a separate
 * place, a PropertyArrayCopy owns the data that it's viewing.
 *
 * Provides utility to retrieve the data stored in the array of elements via the
 * array index operator.
 */
template <typename ElementType> class PropertyArrayCopy {
public:
  /**
   * @brief Constructs an empty array view.
   */
  PropertyArrayCopy() : _storage{}, _view() {}

  /**
   * @brief Constructs an array view from a buffer.
   *
   * @param buffer The buffer containing the values.
   */
  PropertyArrayCopy(const std::vector<ElementType>& values) noexcept
      : _storage(), _view() {
    size_t numberOfElements = values.size();
    size_t sizeInBytes = numberOfElements * sizeof(ElementType);
    this->_storage.resize(sizeInBytes);
    std::memcpy(
        this->_storage.data(),
        reinterpret_cast<const std::byte*>(values.data()),
        sizeInBytes);
    this->_view = PropertyArrayView<ElementType>(this->_storage);
  }

  PropertyArrayCopy(PropertyArrayCopy&&) = default;
  PropertyArrayCopy& operator=(PropertyArrayCopy&&) = default;

  PropertyArrayCopy(std::vector<std::byte>&& buffer) noexcept
      : _storage(std::move(buffer)), _view(this->_storage) {}

  PropertyArrayCopy(const PropertyArrayCopy& rhs)
      : _storage(rhs._storage), _view(this->_storage) {}

  PropertyArrayCopy& operator=(const PropertyArrayCopy& rhs) {
    this->_storage = rhs._storage;
    this->_view = PropertyArrayView<ElementType>(this->_storage);
    return *this;
  }

  const ElementType& operator[](int64_t index) const noexcept {
    return this->_view[index];
  }

  int64_t size() const noexcept { return this->_view.size(); }

  auto begin() { return this->_view.begin(); }
  auto end() { return this->_view.end(); }
  auto begin() const { return this->_view.begin(); }
  auto end() const { return this->_view.end(); }

  const PropertyArrayView<ElementType>& view() const { return this->_view; }

  PropertyArrayView<ElementType>
  toViewAndExternalBuffer(std::vector<std::byte>& outBuffer) && {
    outBuffer = std::move(this->_storage);
    PropertyArrayView<ElementType> result = std::move(this->_view);
    this->_view = PropertyArrayView<ElementType>();
    return result;
  }

private:
  std::vector<std::byte> _storage;
  PropertyArrayView<ElementType> _view;
};

template <> class PropertyArrayView<bool> {
public:
  /**
   * @brief Constructs an empty array view.
   */
  PropertyArrayView() : _values{}, _bitOffset{0}, _size{0} {}

  /**
   * @brief Constructs an array view from a buffer.
   *
   * @param buffer The buffer containing the values.
   * @param bitOffset The offset into the buffer where the values actually
   * begin.
   * @param size The number of values in the array.
   */
  PropertyArrayView(
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

template <> class PropertyArrayView<std::string_view> {
public:
  /**
   * @brief Constructs an empty array view.
   */
  PropertyArrayView()
      : _values{},
        _stringOffsets{},
        _stringOffsetType{PropertyComponentType::None},
        _size{0} {}

  /**
   * @brief Constructs an array view from buffers and their information.
   *
   * @param values The buffer containing the values.
   * @param stringOffsets The buffer containing the string offsets.
   * @param stringOffsetType The component type of the string offsets.
   * @param size The number of values in the array.
   */
  PropertyArrayView(
      const gsl::span<const std::byte>& values,
      const gsl::span<const std::byte>& stringOffsets,
      PropertyComponentType stringOffsetType,
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
  PropertyComponentType _stringOffsetType;
  int64_t _size;
};

template <typename T>
bool operator==(
    const PropertyArrayView<T>& lhs,
    const PropertyArrayView<T>& rhs) {
  int64_t size = lhs.size();
  if (size != rhs.size()) {
    return false;
  }

  for (int64_t i = 0; i < size; ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }

  return true;
}

template <typename T>
bool operator==(
    const PropertyArrayView<T>& lhs,
    const PropertyArrayCopy<T>& rhs) {
  return lhs == PropertyArrayView(rhs);
}

template <typename T>
bool operator==(
    const PropertyArrayCopy<T>& lhs,
    const PropertyArrayView<T>& rhs) {
  return lhs.view() == rhs;
}

template <typename T>
bool operator==(
    const PropertyArrayCopy<T>& lhs,
    const PropertyArrayCopy<T>& rhs) {
  return lhs.view() == rhs.view();
}

template <typename T>
bool operator!=(
    const PropertyArrayView<T>& lhs,
    const PropertyArrayView<T>& rhs) {
  return !(lhs == rhs);
}

template <typename T>
bool operator!=(
    const PropertyArrayView<T>& lhs,
    const PropertyArrayCopy<T>& rhs) {
  return !(lhs == rhs);
}

template <typename T>
bool operator!=(
    const PropertyArrayCopy<T>& lhs,
    const PropertyArrayView<T>& rhs) {
  return !(lhs == rhs);
}

template <typename T>
bool operator!=(
    const PropertyArrayCopy<T>& lhs,
    const PropertyArrayCopy<T>& rhs) {
  return !(lhs == rhs);
}

} // namespace CesiumGltf
