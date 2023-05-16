#pragma once

#include "CesiumGltf/StructuralMetadataArrayView.h"
#include "CesiumGltf/StructuralMetadataPropertyTypeTraits.h"

#include <gsl/span>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace CesiumGltf {
namespace StructuralMetadata {

/**
 * @brief Indicates the status of a property view.
 *
 * The {@link MetadataPropertyView} constructor always completes successfully. However,
 * it may not always reflect the actual content of the {@link ExtensionExtStructuralMetadataPropertyTableProperty}, but
 * instead indicate that its {@link MetadataPropertyView::size} is 0. This enumeration
 * provides the reason.
 */

enum class MetadataPropertyViewStatus {
  /**
   * @brief This property view is valid and ready to use.
   */
  Valid,

  /**
   * @brief This property view does not exist in the
   * ExtensionExtStructuralMetadataPropertyTable.
   */
  InvalidPropertyDoesNotExist,

  /**
   * @brief This property view does not have a correct type with what is
   * specified in {@link ExtensionExtStructuralMetadataClassProperty::type}.
   */
  InvalidTypeMismatch,

  /**
   * @brief This property view does not have a correct component type with what
   * is specified in {@link ExtensionExtStructuralMetadataClassProperty::componentType}.
   */
  InvalidComponentTypeMismatch,

  /**
   * @brief This property view does not have a valid value buffer view index.
   */
  InvalidValueBufferView,

  /**
   * @brief This array property view does not have a valid array offset buffer
   * view index.
   */
  InvalidArrayOffsetBufferViewIndex,

  /**
   * @brief This string property view does not have a valid string offset buffer
   * view index.
   */
  InvalidStringOffsetBufferViewIndex,

  /**
   * @brief This property view has a valid value buffer view, but the buffer
   * view specifies an invalid buffer index.
   */
  InvalidValueBufferIndex,

  /**
   * @brief This property view has a valid array string buffer view, but the
   * buffer view specifies an invalid buffer index.
   */
  InvalidArrayOffsetBufferIndex,

  /**
   * @brief This property view has a valid string offset buffer view, but the
   * buffer view specifies an invalid buffer index.
   */
  InvalidStringOffsetBufferIndex,

  /**
   * @brief This property view has an out-of-bounds buffer view
   */
  InvalidBufferViewOutOfBounds,

  /**
   * @brief This property view has an invalid buffer view's length which is not
   * a multiple of the size of its type or offset type
   */
  InvalidBufferViewSizeNotDivisibleByTypeSize,

  /**
   * @brief This property view has an invalid buffer view's length which cannot
   * fit all the instances of the feature table
   */
  InvalidBufferViewSizeNotFitInstanceCount,

  /**
   * @brief This array property view has both count and offset buffer
   * view defined.
   */
  InvalidArrayCountAndOffsetBufferCoexist,

  /**
   * @brief This array property view doesn't have count nor offset buffer view
   * defined.
   */
  InvalidArrayCountAndOffsetBufferDontExist,

  /**
   * @brief This property view has an unknown array offset type.
   */
  InvalidArrayOffsetType,

  /**
   * @brief This property view has an unknown string offset type.
   */
  InvalidStringOffsetType,

  /**
   * @brief This property view's array offset values are not sorted in ascending
   * order.
   */
  InvalidArrayOffsetValuesNotSorted,

  /**
   * @brief This property view's string offset values are not sorted in
   * ascending order.
   */
  InvalidStringOffsetValuesNotSorted,

  /**
   * @brief This property view has an array offset that is out of bounds.
   */
  InvalidArrayOffsetValueOutOfBounds,

  /**
   * @brief This property view has a string offset that is out of bounds.
   */
  InvalidStringOffsetValueOutOfBounds
};

/**
 * @brief A view on the data of the
 * ExtensionExtStructuralMetadataPropertyTableProperty
 *
 * It provides utility to retrieve the actual data stored in the
 * {@link ExtensionExtStructuralMetadataPropertyTableProperty::values} like an array of elements.
 * Data of each instance can be accessed through the {@link get(int64_t instance)} method
 *
 * @param ElementType must be a scalar (uin8_t, int8_t, uint16_t, int16_t,
 * uint32_t, int32_t, uint64_t, int64_t, float, double), vecN, matN, bool,
 * std::string_view, or MetadataArrayView<T> with T as one of the aforementioned
 * types.
 */
template <typename ElementType> class MetadataPropertyView {
public:
  /**
   * @brief Constructs a new instance viewing a non-existent property.
   */
  MetadataPropertyView()
      : _status{MetadataPropertyViewStatus::InvalidPropertyDoesNotExist},
        _values{},
        _componentCount{},
        _instanceCount{},
        _normalized{} {}

  /**
   * @brief Construct a new instance pointing to the data specified by
   * ExtensionExtStructuralMetadataPropertyTableProperty
   * @param values The raw buffer specified by {@link ExtensionExtStructuralMetadataPropertyTableProperty::values}
   * @param arrayOffsets The raw buffer specified by {@link ExtensionExtStructuralMetadataPropertyTableProperty::arrayOffsets}
   * @param stringOffsets The raw buffer specified by {@link ExtensionExtStructuralMetadataPropertyTableProperty::stringOffsets}
   * @param offsetType The offset type of arrayOffsets specified by {@link ExtensionExtStructuralMetadataPropertyTableProperty::arrayOffsetType}
   * @param offsetType The offset type of stringOffsets specified by {@link ExtensionExtStructuralMetadataPropertyTableProperty::stringOffsetType}
   * @param fixedArrayCount The number of elements in each array value specified by {@link ExtensionExtStructuralMetadataClassProperty::count}
   * @param size The number of elements in the property table specified by {@link ExtensionExtStructuralMetadataPropertyTable::count}
   * @param normalized Whether this property has a normalized integer type.
   */
  MetadataPropertyView(
      MetadataPropertyViewStatus status,
      gsl::span<const std::byte> values,
      gsl::span<const std::byte> arrayOffsets,
      gsl::span<const std::byte> stringOffsets,
      StructuralMetadata::PropertyComponentType arrayOffsetType,
      StructuralMetadata::PropertyComponentType stringOffsetType,
      int64_t fixedLengthArrayCount,
      int64_t size,
      bool normalized) noexcept
      : _status{status},
        _values{values},
        _arrayOffsets{arrayOffsets},
        _arrayOffsetType{arrayOffsetType},
        _arrayOffsetTypeSize{getOffsetTypeSize(arrayOffsetType)},
        _stringOffsets{stringOffsets},
        _stringOffsetType{stringOffsetType},
        _stringOffsetTypeSize{getOffsetTypeSize(stringOffsetType)},
        _fixedLengthArrayCount{fixedLengthArrayCount},
        _size{size},
        _normalized{normalized} {}

  /**
   * @brief Gets the status of this property view.
   *
   * Indicates whether the view accurately reflects the property's data, or
   * whether an error occurred.
   */
  MetadataPropertyViewStatus status() const noexcept { return _status; }

  /**
   * @brief Get the value of an element of the FeatureTable.
   * @param index The element index
   * @return The value of the element
   */
  ElementType get(int64_t index) const noexcept {
    assert(
        _status == MetadataPropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");
    assert(
        size() > 0 &&
        "Check the size() of the view to make sure it's not empty");
    assert(index >= 0 && "index must be positive");

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
   * @brief Get the number of elements in the
   * ExtensionExtStructuralMetadataPropertyTable.
   *
   * @return The number of elements in the
   * ExtensionExtStructuralMetadataPropertyTable.
   */
  int64_t size() const noexcept { return _size; }

  /**
   * @brief Get the element count of the fixed-length arrays in this property.
   * Only applicable when the property is an array type.
   *
   * @return The count of this property.
   */
  int64_t getFixedLengthArrayCount() const noexcept { return _fixedArrayCount; }

  /**
   * @brief Whether this property has a normalized integer type.
   *
   * @return Whether this property has a normalized integer type.
   */
  bool isNormalized() const noexcept { return _normalized; }

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
  MetadataArrayView<T> getNumericArrayValues(int64_t index) const noexcept {
    // Handle fixed-length arrays
    if (_fixedLengthArrayCount > 0) {
      size_t arraySize = _fixedLengthArrayCount * sizeof(T);
      const gsl::span<const std::byte> values(
          _values.data() + index * arraySize,
          arraySize);
      return MetadataArrayView<T>{values};
    }

    // Handle variable-length arrays
    const size_t currentOffset =
        getOffsetFromOffsetsBuffer(index, _arrayOffsets, _arrayOffsetType);
    const size_t nextOffset =
        getOffsetFromOffsetsBuffer(index + 1, _arrayOffsets, _arrayOffsetType);
    const gsl::span<const std::byte> values(
        _values.data() + currentOffset,
        nextOffset - currentOffset);
    return MetadataArrayView<T>{values};
  }

  MetadataArrayView<std::string_view>
  getStringArrayValues(int64_t index) const noexcept {
    // Handle fixed-length arrays
    if (_fixedLengthArrayCount > 0) {
      size_t arraySize = _fixedLengthArrayCount * _stringOffsetTypeSize;
      const gsl::span<const std::byte> stringOffsetValues(
          _stringOffsets.data() + index * arraySize,
          arraySize);
      return MetadataArrayView<std::string_view>(
          _values,
          stringOffsetValues,
          _stringOffsetType,
          _count);
    }

    const size_t currentArrayOffset =
        getOffsetFromOffsetsBuffer(index, _arrayOffsets, _arrayOffsetType);
    const size_t nextArrayOffset =
        getOffsetFromOffsetsBuffer(index + 1, _arrayOffsets, _arrayOffsetType);
    const size_t length = currentArrayOffset - nextArrayOffset;
    const gsl::span<const std::byte> stringOffsetValues(
        _stringOffsets.data() + currentOffset,
        length * _arrayOffsetTypeSize);
    return MetadataArrayView<std::string_view>(
        _values,
        stringOffsetValues,
        _stringOffsetType,
        length / _stringOffsetTypeSize);
  }

  MetadataArrayView<bool> getBooleanArrayValues(int64_t index) const noexcept {
    // Handle fixed-length arrays
    if (_fixedLengthArrayCount > 0) {
      const size_t offsetBits = _fixedLengthArrayCount * index;
      const size_t nextOffsetBits = _fixedLengthArrayCount * (index + 1);
      const gsl::span<const std::byte> buffer(
          _values.data() + offsetBits / 8,
          (nextOffsetBits / 8 - offsetBits / 8 + 1));
      return MetadataArrayView<bool>(
          buffer,
          offsetBits % 8,
          _fixedLengthArrayCount);
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
    return MetadataArrayView<bool>(buffer, currentOffset % 8, totalBits);
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

  MetadataPropertyViewStatus _status;
  gsl::span<const std::byte> _values;

  gsl::span<const std::byte> _arrayOffsets;
  PropertyComponentType _arrayOffsetType;
  int64_t _arrayOffsetTypeSize;

  gsl::span<const std::byte> _stringOffsets;
  PropertyComponentType _stringOffsetType;
  int64_t _stringOffsetTypeSize;

  int64_t _fixedLengthArrayCount;
  int64_t _size;
  bool _normalized;
};

} // namespace StructuralMetadata
} // namespace CesiumGltf