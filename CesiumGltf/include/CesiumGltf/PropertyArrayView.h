#pragma once

#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/getOffsetFromOffsetsBuffer.h>
#include <CesiumUtility/SpanHelper.h>

#include <cstddef>
#include <cstring>
#include <span>
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
  PropertyArrayView(const std::span<const std::byte>& buffer) noexcept
      : _values{CesiumUtility::reintepretCastSpan<const ElementType>(buffer)} {}

  /**
   * @brief Accesses the element of this array at the given index.
   */
  const ElementType& operator[](int64_t index) const noexcept {
    return this->_values[index];
  }

  /**
   * @brief The number of elements in this array.
   */
  int64_t size() const noexcept { return this->_values.size(); }

  /**
   * @brief The `begin` iterator.
   */
  auto begin() { return this->_values.begin(); }
  /**
   * @brief The `end` iterator.
   */
  auto end() { return this->_values.end(); }
  /** @copydoc begin */
  auto begin() const { return this->_values.begin(); }
  /** @copydoc end */
  auto end() const { return this->_values.end(); }

private:
  std::span<const ElementType> _values;
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
   * @param values The buffer containing the values.
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

  /** @brief Default move constructor */
  PropertyArrayCopy(PropertyArrayCopy&&) = default;
  /** @brief Default move assignment operator */
  PropertyArrayCopy& operator=(PropertyArrayCopy&&) = default;

  /** @brief Creates a new \ref PropertyArrayCopy directly from a buffer of
   * bytes, which will be moved into this copy. */
  PropertyArrayCopy(std::vector<std::byte>&& buffer) noexcept
      : _storage(std::move(buffer)), _view(this->_storage) {}

  /** @brief Copy constructor */
  PropertyArrayCopy(const PropertyArrayCopy& rhs)
      : _storage(rhs._storage), _view(this->_storage) {}

  /** @brief Copy assignment operator */
  PropertyArrayCopy& operator=(const PropertyArrayCopy& rhs) {
    this->_storage = rhs._storage;
    this->_view = PropertyArrayView<ElementType>(this->_storage);
    return *this;
  }

  /**
   * @brief Returns the `ElementType` at the given index from this copy.
   *
   * @param index The index to obtain.
   * @returns The `ElementType` at that index from the internal view.
   */
  const ElementType& operator[](int64_t index) const noexcept {
    return this->_view[index];
  }

  /** @copydoc PropertyArrayView::size */
  int64_t size() const noexcept { return this->_view.size(); }

  /** @copydoc PropertyArrayView::begin */
  auto begin() { return this->_view.begin(); }
  /** @copydoc PropertyArrayView::end */
  auto end() { return this->_view.end(); }
  /** @copydoc PropertyArrayView::begin */
  auto begin() const { return this->_view.begin(); }
  /** @copydoc PropertyArrayView::end */
  auto end() const { return this->_view.end(); }

  /**
   * @brief Obtains a \ref PropertyArrayView over the contents of this copy.
   */
  const PropertyArrayView<ElementType>& view() const { return this->_view; }

  /**
   * @brief Obtains a buffer and view from the copied data, leaving this \ref
   * PropertyArrayCopy empty.
   *
   * @param outBuffer The destination where this copy's internal buffer will be
   * moved to.
   */
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

/**
 * @brief A view on a bool array element of a {@link PropertyTableProperty}
 * or {@link PropertyTextureProperty}.
 *
 * Provides utility to retrieve the data stored in the array of
 * elements via the array index operator.
 */
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
      const std::span<const std::byte>& buffer,
      int64_t bitOffset,
      int64_t size) noexcept
      : _values{buffer}, _bitOffset{bitOffset}, _size{size} {}

  /**
   * @brief Obtains the element in the array at the given index.
   */
  bool operator[](int64_t index) const noexcept {
    index += _bitOffset;
    const int64_t byteIndex = index / 8;
    const int64_t bitIndex = index % 8;
    const int bitValue = static_cast<int>(_values[byteIndex] >> bitIndex) & 1;
    return bitValue == 1;
  }

  /**
   * @brief The number of entries in the array.
   */
  int64_t size() const noexcept { return _size; }

private:
  std::span<const std::byte> _values;
  int64_t _bitOffset;
  int64_t _size;
};

/**
 * @brief A view on a string array element of a {@link PropertyTableProperty}
 * or {@link PropertyTextureProperty}.
 *
 * Provides utility to retrieve the data stored in the array of
 * elements via the array index operator.
 */
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
      const std::span<const std::byte>& values,
      const std::span<const std::byte>& stringOffsets,
      PropertyComponentType stringOffsetType,
      int64_t size) noexcept
      : _values{values},
        _stringOffsets{stringOffsets},
        _stringOffsetType{stringOffsetType},
        _size{size} {}

  /**
   * @brief Obtains an `std::string_view` for the element at the given index.
   */
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

  /**
   * @brief The number of elements in this array.
   */
  int64_t size() const noexcept { return _size; }

private:
  std::span<const std::byte> _values;
  std::span<const std::byte> _stringOffsets;
  PropertyComponentType _stringOffsetType;
  int64_t _size;
};

/** @brief Compares two \ref PropertyArrayView instances by comparing their
 * values. If the two arrays aren't the same size, this comparison will return
 * false. */
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

/** @brief Compares a \ref PropertyArrayView with a \ref
 * PropertyArrayCopy by creating a view from the copy and comparing the two. */
template <typename T>
bool operator==(
    const PropertyArrayView<T>& lhs,
    const PropertyArrayCopy<T>& rhs) {
  return lhs == PropertyArrayView(rhs);
}

/** @brief Compares a \ref PropertyArrayView with a \ref
 * PropertyArrayCopy by creating a view from the copy and comparing the two. */
template <typename T>
bool operator==(
    const PropertyArrayCopy<T>& lhs,
    const PropertyArrayView<T>& rhs) {
  return lhs.view() == rhs;
}

/** @brief Compares two \ref PropertyArrayCopy instances by creating
 * views from each instance and comparing the two. */
template <typename T>
bool operator==(
    const PropertyArrayCopy<T>& lhs,
    const PropertyArrayCopy<T>& rhs) {
  return lhs.view() == rhs.view();
}

/**
 * @brief Compares two \ref PropertyArrayView instances and returns the inverse.
 */
template <typename T>
bool operator!=(
    const PropertyArrayView<T>& lhs,
    const PropertyArrayView<T>& rhs) {
  return !(lhs == rhs);
}

/** @brief Compares a \ref PropertyArrayView with a \ref
 * PropertyArrayCopy by creating a view from the copy and returning the inverse
 * of comparing the two. */
template <typename T>
bool operator!=(
    const PropertyArrayView<T>& lhs,
    const PropertyArrayCopy<T>& rhs) {
  return !(lhs == rhs);
}

/** @brief Compares a \ref PropertyArrayView with a \ref
 * PropertyArrayCopy by creating a view from the copy and returning the inverse
 * of comparing the two. */
template <typename T>
bool operator!=(
    const PropertyArrayCopy<T>& lhs,
    const PropertyArrayView<T>& rhs) {
  return !(lhs == rhs);
}

/** @brief Compares two \ref
 * PropertyArrayCopy instances by creating views from both instances and
 * returning the inverse of comparing the two. */
template <typename T>
bool operator!=(
    const PropertyArrayCopy<T>& lhs,
    const PropertyArrayCopy<T>& rhs) {
  return !(lhs == rhs);
}

} // namespace CesiumGltf
