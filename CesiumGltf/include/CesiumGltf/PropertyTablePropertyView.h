#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumGltf/PropertyView.h"

#include <gsl/span>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace CesiumGltf {
/**
 * @brief Indicates the status of a property table property view.
 *
 * The {@link PropertyTablePropertyView} constructor always completes successfully.
 * However, it may not always reflect the actual content of the {@link PropertyTableProperty},
 * but instead indicate that its {@link PropertyTablePropertyView::size} is 0.
 * This enumeration provides the reason.
 */
class PropertyTablePropertyViewStatus : public PropertyViewStatus {
public:
  /**
   * @brief This property view was initialized from an invalid {@link PropertyTable}.
   */
  static const PropertyViewStatusType ErrorInvalidPropertyTable = 12;

  /**
   * @brief This property view does not have a valid value buffer view index.
   */
  static const PropertyViewStatusType ErrorInvalidValueBufferView = 13;

  /**
   * @brief This array property view does not have a valid array offset buffer
   * view index.
   */
  static const PropertyViewStatusType ErrorInvalidArrayOffsetBufferView = 14;

  /**
   * @brief This string property view does not have a valid string offset buffer
   * view index.
   */
  static const PropertyViewStatusType ErrorInvalidStringOffsetBufferView = 15;

  /**
   * @brief This property view has a valid value buffer view, but the buffer
   * view specifies an invalid buffer index.
   */
  static const PropertyViewStatusType ErrorInvalidValueBuffer = 16;

  /**
   * @brief This property view has a valid array string buffer view, but the
   * buffer view specifies an invalid buffer index.
   */
  static const PropertyViewStatusType ErrorInvalidArrayOffsetBuffer = 17;

  /**
   * @brief This property view has a valid string offset buffer view, but the
   * buffer view specifies an invalid buffer index.
   */
  static const PropertyViewStatusType ErrorInvalidStringOffsetBuffer = 18;

  /**
   * @brief This property view has a buffer view that points outside the bounds
   * of its target buffer.
   */
  static const PropertyViewStatusType ErrorBufferViewOutOfBounds = 19;

  /**
   * @brief This property view has an invalid buffer view; its length is not
   * a multiple of the size of its type / offset type.
   */
  static const PropertyViewStatusType
      ErrorBufferViewSizeNotDivisibleByTypeSize = 20;

  /**
   * @brief This property view has an invalid buffer view; its length does not
   * match the size of the property table.
   */
  static const PropertyViewStatusType
      ErrorBufferViewSizeDoesNotMatchPropertyTableCount = 21;

  /**
   * @brief This array property view has both a fixed length and an offset
   * buffer view defined.
   */
  static const PropertyViewStatusType ErrorArrayCountAndOffsetBufferCoexist =
      22;

  /**
   * @brief This array property view has neither a fixed length nor an offset
   * buffer view defined.
   */
  static const PropertyViewStatusType ErrorArrayCountAndOffsetBufferDontExist =
      23;

  /**
   * @brief This property view has an unknown array offset type.
   */
  static const PropertyViewStatusType ErrorInvalidArrayOffsetType = 24;

  /**
   * @brief This property view has an unknown string offset type.
   */
  static const PropertyViewStatusType ErrorInvalidStringOffsetType = 25;

  /**
   * @brief This property view's array offset values are not sorted in ascending
   * order.
   */
  static const PropertyViewStatusType ErrorArrayOffsetsNotSorted = 26;

  /**
   * @brief This property view's string offset values are not sorted in
   * ascending order.
   */
  static const PropertyViewStatusType ErrorStringOffsetsNotSorted = 27;

  /**
   * @brief This property view has an array offset that is out of bounds.
   */
  static const PropertyViewStatusType ErrorArrayOffsetOutOfBounds = 28;

  /**
   * @brief This property view has a string offset that is out of bounds.
   */
  static const PropertyViewStatusType ErrorStringOffsetOutOfBounds = 29;
};

template <typename ElementType, bool Normalized = false, typename Enable = void>
class PropertyTablePropertyView;

/**
 * @brief A view on the data of the {@link PropertyTableProperty} that is created
 * by a {@link PropertyTableView}.
 *
 * It provides utility to retrieve the actual data stored in the
 * {@link PropertyTableProperty::values} like an array of elements. Data of each
 * instance can be accessed through the {@link PropertyTablePropertyView::get} method.
 *
 * @param ElementType must be one of the following: a scalar (uint8_t, int8_t,
 * uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, float, double), a
 * glm vecN composed of one of the scalar types, a glm matN composed of one of
 * the scalar types, bool, std::string_view, or PropertyArrayView<T> with T as
 * one of the aforementioned types.
 */
template <typename ElementType>
class PropertyTablePropertyView<ElementType>
    : public PropertyView<ElementType> {
public:
  /**
   * @brief Constructs an invalid instance for a non-existent property.
   */
  PropertyTablePropertyView()
      : PropertyView<ElementType>(),
        _values{},
        _size{0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0},
        _stringOffsets{},
        _stringOffsetType{PropertyComponentType::None},
        _stringOffsetTypeSize{0} {}

  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyTablePropertyViewStatus} indicating the error with the property.
   */
  PropertyTablePropertyView(PropertyViewStatusType status)
      : PropertyView<ElementType>(status),
        _values{},
        _size{0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0},
        _stringOffsets{},
        _stringOffsetType{PropertyComponentType::None},
        _stringOffsetTypeSize{0} {
    assert(
        this->_status != PropertyTablePropertyViewStatus::Valid &&
        "An empty property view should not be constructed with a valid status");
  }

  /**
   * @brief Construct an instance pointing to non-array data specified by a {@link PropertyTableProperty}.
   *
   * @param property The {@link PropertyTableProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   * @param value The raw buffer specified by {@link PropertyTableProperty::values}
   */
  PropertyTablePropertyView(
      const PropertyTableProperty& property,
      const ClassProperty& classProperty,
      int64_t size,
      gsl::span<const std::byte> values) noexcept
      : PropertyView<ElementType>(classProperty, property),
        _values{values},
        _size{
            this->_status == PropertyTablePropertyViewStatus::Valid ? size : 0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0},
        _stringOffsets{},
        _stringOffsetType{PropertyComponentType::None},
        _stringOffsetTypeSize{0} {}

  /**
   * @brief Construct an instance pointing to the data specified by a {@link PropertyTableProperty}.
   *
   *
   * @param property The {@link PropertyTableProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   * @param values The raw buffer specified by {@link PropertyTableProperty::values}
   * @param arrayOffsets The raw buffer specified by {@link PropertyTableProperty::arrayOffsets}
   * @param stringOffsets The raw buffer specified by {@link PropertyTableProperty::stringOffsets}
   * @param offsetType The offset type of arrayOffsets specified by {@link PropertyTableProperty::arrayOffsetType}
   * @param offsetType The offset type of stringOffsets specified by {@link PropertyTableProperty::stringOffsetType}
   */
  PropertyTablePropertyView(
      const PropertyTableProperty& property,
      const ClassProperty& classProperty,
      int64_t size,
      gsl::span<const std::byte> values,
      gsl::span<const std::byte> arrayOffsets,
      gsl::span<const std::byte> stringOffsets,
      PropertyComponentType arrayOffsetType,
      PropertyComponentType stringOffsetType) noexcept
      : PropertyView<ElementType>(classProperty, property),
        _values{values},
        _size{
            this->_status == PropertyTablePropertyViewStatus::Valid ? size : 0},
        _arrayOffsets{arrayOffsets},
        _arrayOffsetType{arrayOffsetType},
        _arrayOffsetTypeSize{getOffsetTypeSize(arrayOffsetType)},
        _stringOffsets{stringOffsets},
        _stringOffsetType{stringOffsetType},
        _stringOffsetTypeSize{getOffsetTypeSize(stringOffsetType)} {}

  /**
   * @brief Get the raw value of an element of the {@link PropertyTable},
   * without offset, scale, or normalization applied.
   *
   * If this property has a "no data" value defined, the raw value will still be
   * returned, even if it equals the "no data" value.
   *
   * @param index The element index
   * @return The value of the element
   */
  ElementType get(int64_t index) const noexcept {
    assert(
        this->_status == PropertyTablePropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");
    assert(
        size() > 0 &&
        "Check the size() of the view to make sure it's not empty");
    assert(index >= 0 && "index must be non-negative");
    assert(index < size() && "index must be less than size");

    if constexpr (IsMetadataNumeric<ElementType>::value) {
      return getNumericValue(index);
    }

    if constexpr (IsMetadataBoolean<ElementType>::value) {
      return getBooleanValue(index);
    }

    if constexpr (IsMetadataString<ElementType>::value) {
      return getStringValue(index);
    }

    if constexpr (IsMetadataNumericArray<ElementType>::value) {
      return getNumericArrayValues<
          typename MetadataArrayType<ElementType>::type>(index);
    }

    if constexpr (IsMetadataBooleanArray<ElementType>::value) {
      return getBooleanArrayValues(index);
    }

    if constexpr (IsMetadataStringArray<ElementType>::value) {
      return getStringArrayValues(index);
    }
  }

  /**
   * @brief Get the number of elements in this
   * PropertyTablePropertyView. If the view is valid, this returns
   * {@link PropertyTable::count}. Otherwise, this returns 0.
   *
   * @return The number of elements in this PropertyTablePropertyView.
   */
  int64_t size() const noexcept {
    return this->_status == PropertyTablePropertyViewStatus::Valid ? _size : 0;
  }

private:
  ElementType getNumericValue(int64_t index) const noexcept {
    return reinterpret_cast<const ElementType*>(_values.data())[index];
  }

  bool getBooleanValue(int64_t index) const noexcept {
    const int64_t byteIndex = index / 8;
    const int64_t bitIndex = index % 8;
    const int bitValue = static_cast<int>(_values[byteIndex] >> bitIndex) & 1;
    return bitValue == 1;
  }

  std::string_view getStringValue(int64_t index) const noexcept {
    const size_t currentOffset =
        getOffsetFromOffsetsBuffer(index, _stringOffsets, _stringOffsetType);
    const size_t nextOffset = getOffsetFromOffsetsBuffer(
        index + 1,
        _stringOffsets,
        _stringOffsetType);
    return std::string_view(
        reinterpret_cast<const char*>(_values.data() + currentOffset),
        nextOffset - currentOffset);
  }

  template <typename T>
  PropertyArrayView<T> getNumericArrayValues(int64_t index) const noexcept {
    size_t count = static_cast<size_t>(this->arrayCount());
    // Handle fixed-length arrays
    if (count > 0) {
      size_t arraySize = count * sizeof(T);
      const gsl::span<const std::byte> values(
          _values.data() + index * arraySize,
          arraySize);
      return PropertyArrayView<T>{values};
    }

    // Handle variable-length arrays
    const size_t currentOffset =
        getOffsetFromOffsetsBuffer(index, _arrayOffsets, _arrayOffsetType);
    const size_t nextOffset =
        getOffsetFromOffsetsBuffer(index + 1, _arrayOffsets, _arrayOffsetType);
    const gsl::span<const std::byte> values(
        _values.data() + currentOffset,
        nextOffset - currentOffset);
    return PropertyArrayView<T>{values};
  }

  PropertyArrayView<std::string_view>
  getStringArrayValues(int64_t index) const noexcept {
    size_t count = static_cast<size_t>(this->arrayCount());
    // Handle fixed-length arrays
    if (count > 0) {
      // Copy the corresponding string offsets to pass to the PropertyArrayView.
      const size_t arraySize = count * _stringOffsetTypeSize;
      const gsl::span<const std::byte> stringOffsetValues(
          _stringOffsets.data() + index * arraySize,
          arraySize + _stringOffsetTypeSize);
      return PropertyArrayView<std::string_view>(
          _values,
          stringOffsetValues,
          _stringOffsetType,
          count);
    }

    // Handle variable-length arrays
    const size_t currentArrayOffset =
        getOffsetFromOffsetsBuffer(index, _arrayOffsets, _arrayOffsetType);
    const size_t nextArrayOffset =
        getOffsetFromOffsetsBuffer(index + 1, _arrayOffsets, _arrayOffsetType);
    const size_t arraySize = nextArrayOffset - currentArrayOffset;
    const gsl::span<const std::byte> stringOffsetValues(
        _stringOffsets.data() + currentArrayOffset,
        arraySize + _arrayOffsetTypeSize);
    return PropertyArrayView<std::string_view>(
        _values,
        stringOffsetValues,
        _stringOffsetType,
        arraySize / _arrayOffsetTypeSize);
  }

  PropertyArrayView<bool> getBooleanArrayValues(int64_t index) const noexcept {
    size_t count = static_cast<size_t>(this->arrayCount());
    // Handle fixed-length arrays
    if (count > 0) {
      const size_t offsetBits = count * index;
      const size_t nextOffsetBits = count * (index + 1);
      const gsl::span<const std::byte> buffer(
          _values.data() + offsetBits / 8,
          (nextOffsetBits / 8 - offsetBits / 8 + 1));
      return PropertyArrayView<bool>(buffer, offsetBits % 8, count);
    }

    // Handle variable-length arrays
    const size_t currentOffset =
        getOffsetFromOffsetsBuffer(index, _arrayOffsets, _arrayOffsetType);
    const size_t nextOffset =
        getOffsetFromOffsetsBuffer(index + 1, _arrayOffsets, _arrayOffsetType);
    const size_t totalBits = nextOffset - currentOffset;
    const gsl::span<const std::byte> buffer(
        _values.data() + currentOffset / 8,
        (nextOffset / 8 - currentOffset / 8 + 1));
    return PropertyArrayView<bool>(buffer, currentOffset % 8, totalBits);
  }

  static int64_t getOffsetTypeSize(PropertyComponentType offsetType) noexcept {
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

  gsl::span<const std::byte> _values;
  int64_t _size;

  gsl::span<const std::byte> _arrayOffsets;
  PropertyComponentType _arrayOffsetType;
  int64_t _arrayOffsetTypeSize;

  gsl::span<const std::byte> _stringOffsets;
  PropertyComponentType _stringOffsetType;
  int64_t _stringOffsetTypeSize;
};
} // namespace CesiumGltf
