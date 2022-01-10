#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/PropertyTypeTraits.h"

#include <gsl/span>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace CesiumGltf {
/**
 * @brief Indicates the status of a property view.
 *
 * The {@link MetadataPropertyView} constructor always completes successfully. However,
 * it may not always reflect the actual content of the {@link FeatureTableProperty}, but
 * instead indicate that its {@link MetadataPropertyView::size} is 0. This enumeration
 * provides the reason.
 */
enum class MetadataPropertyViewStatus {
  /**
   * @brief This property view is valid and ready to use.
   */
  Valid,

  /**
   * @brief This property view does not exist in the FeatureTable.
   */
  InvalidPropertyNotExist,

  /**
   * @brief This property view does not have a correct type with what is
   * specified in {@link ClassProperty::type}.
   */
  InvalidTypeMismatch,

  /**
   * @brief This property view does not have a valid value buffer view index.
   */
  InvalidValueBufferViewIndex,

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
   * @brief This property view has a valid value buffer view index, but buffer
   * view specifies an invalid buffer index
   */
  InvalidValueBufferIndex,

  /**
   * @brief This property view has a valid array string buffer view index, but
   * buffer view specifies an invalid buffer index
   */
  InvalidArrayOffsetBufferIndex,

  /**
   * @brief This property view has a valid string offset buffer view index, but
   * buffer view specifies an invalid buffer index
   */
  InvalidStringOffsetBufferIndex,

  /**
   * @brief This property view has buffer view's offset not aligned by 8 bytes
   */
  InvalidBufferViewNotAligned8Bytes,

  /**
   * @brief This property view has an out-of-bound buffer view
   */
  InvalidBufferViewOutOfBound,

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
   * @brief This array property view has both component count and offset buffer
   * view
   */
  InvalidArrayComponentCountAndOffsetBufferCoexist,

  /**
   * @brief This array property view doesn't have either component count or
   * offset buffer view
   */
  InvalidArrayComponentCountOrOffsetBufferNotExist,

  /**
   * @brief This property view have an unknown offset type
   */
  InvalidOffsetType,

  /**
   * @brief This property view has offset values not sorted ascendingly
   */
  InvalidOffsetValuesNotSortedAscending,

  /**
   * @brief This property view has an offset point to an out of bound value
   */
  InvalidOffsetValuePointsToOutOfBoundBuffer
};

/**
 * @brief A view on the data of the FeatureTableProperty
 *
 * It provides utility to retrieve the actual data stored in the
 * {@link FeatureTableProperty::bufferView} like an array of elements.
 * Data of each instance can be accessed through the {@link get(int64_t instance)} method
 *
 * @param ElementType must be uin8_t, int8_t, uint16_t, int16_t,
 * uint32_t, int32_t, uint64_t, int64_t, float, double, bool, std::string_view,
 * and MetadataArrayView<T> with T must be one of the types mentioned above
 */
template <typename ElementType> class MetadataPropertyView {
public:
  /**
   * @brief Constructs a new instance viewing a non-existent property.
   */
  MetadataPropertyView()
      : _status{MetadataPropertyViewStatus::InvalidPropertyNotExist},
        _valueBuffer{},
        _arrayOffsetBuffer{},
        _stringOffsetBuffer{},
        _offsetType{},
        _offsetSize{},
        _componentCount{},
        _instanceCount{},
        _normalized{} {}

  /**
   * @brief Construct a new instance pointing to the data specified by
   * FeatureTableProperty
   * @param valueBuffer The raw buffer specified by {@link FeatureTableProperty::bufferView}
   * @param arrayOffsetBuffer The raw buffer specified by {@link FeatureTableProperty::arrayOffsetBufferView}
   * @param stringOffsetBuffer The raw buffer specified by {@link FeatureTableProperty::stringOffsetBufferView}
   * @param offsetType The offset type of the arrayOffsetBuffer and stringOffsetBuffer that is specified by {@link FeatureTableProperty::offsetType}
   * @param componentCount The number of elements for fixed array value which is specified by {@link FeatureTableProperty::componentCount}
   * @param instanceCount The number of instances specified by {@link FeatureTable::count}
   * @param normalized Whether this property has a normalized integer type.
   */
  MetadataPropertyView(
      MetadataPropertyViewStatus status,
      gsl::span<const std::byte> valueBuffer,
      gsl::span<const std::byte> arrayOffsetBuffer,
      gsl::span<const std::byte> stringOffsetBuffer,
      PropertyType offsetType,
      int64_t componentCount,
      int64_t instanceCount,
      bool normalized) noexcept
      : _status{status},
        _valueBuffer{valueBuffer},
        _arrayOffsetBuffer{arrayOffsetBuffer},
        _stringOffsetBuffer{stringOffsetBuffer},
        _offsetType{offsetType},
        _offsetSize{getOffsetSize(offsetType)},
        _componentCount{componentCount},
        _instanceCount{instanceCount},
        _normalized{normalized} {}

  /**
   * @brief Gets the status of this property view.
   *
   * Indicates whether the view accurately reflects the property's data, or
   * whether an error occurred.
   */
  MetadataPropertyViewStatus status() const noexcept { return _status; }

  /**
   * @brief Get the value of an instance of the FeatureTable.
   * @param instance The instance index
   * @return The value of the instance
   */
  ElementType get(int64_t instance) const noexcept {
    assert(
        _status == MetadataPropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");
    assert(
        size() > 0 &&
        "Check the size() of the view to make sure it's not empty");
    assert(instance >= 0 && "instance index must be positive");

    if constexpr (IsMetadataNumeric<ElementType>::value) {
      return getNumeric(instance);
    }

    if constexpr (IsMetadataBoolean<ElementType>::value) {
      return getBoolean(instance);
    }

    if constexpr (IsMetadataString<ElementType>::value) {
      return getString(instance);
    }

    if constexpr (IsMetadataNumericArray<ElementType>::value) {
      return getNumericArray<typename MetadataArrayType<ElementType>::type>(
          instance);
    }

    if constexpr (IsMetadataBooleanArray<ElementType>::value) {
      return getBooleanArray(instance);
    }

    if constexpr (IsMetadataStringArray<ElementType>::value) {
      return getStringArray(instance);
    }
  }

  /**
   * @brief Get the number of instances in the FeatureTable
   * @return The number of instances in the FeatureTable
   */
  int64_t size() const noexcept { return _instanceCount; }

  /**
   * @brief Get the component count of this property. Only applicable when the
   * property is an array type.
   *
   * @return The component count of this property.
   */
  int64_t getComponentCount() const noexcept { return _componentCount; }

  /**
   * @brief Whether this property has a normalized integer type.
   *
   * @return Whether this property has a normalized integer type.
   */
  bool isNormalized() const noexcept { return _normalized; }

private:
  ElementType getNumeric(int64_t instance) const noexcept {
    return reinterpret_cast<const ElementType*>(_valueBuffer.data())[instance];
  }

  bool getBoolean(int64_t instance) const noexcept {
    const int64_t byteIndex = instance / 8;
    const int64_t bitIndex = instance % 8;
    const int bitValue =
        static_cast<int>(_valueBuffer[byteIndex] >> bitIndex) & 1;
    return bitValue == 1;
  }

  std::string_view getString(int64_t instance) const noexcept {
    const size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _stringOffsetBuffer, _offsetType);
    const size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _stringOffsetBuffer,
        _offsetType);
    return std::string_view(
        reinterpret_cast<const char*>(_valueBuffer.data() + currentOffset),
        (nextOffset - currentOffset));
  }

  template <typename T>
  MetadataArrayView<T> getNumericArray(int64_t instance) const noexcept {
    if (_componentCount > 0) {
      const gsl::span<const std::byte> vals(
          _valueBuffer.data() + instance * _componentCount * sizeof(T),
          _componentCount * sizeof(T));
      return MetadataArrayView<T>{vals};
    }

    const size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _arrayOffsetBuffer, _offsetType);
    const size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _arrayOffsetBuffer,
        _offsetType);
    const gsl::span<const std::byte> vals(
        _valueBuffer.data() + currentOffset,
        (nextOffset - currentOffset));
    return MetadataArrayView<T>{vals};
  }

  MetadataArrayView<std::string_view>
  getStringArray(int64_t instance) const noexcept {
    if (_componentCount > 0) {
      const gsl::span<const std::byte> offsetVals(
          _stringOffsetBuffer.data() + instance * _componentCount * _offsetSize,
          (_componentCount + 1) * _offsetSize);
      return MetadataArrayView<std::string_view>(
          _valueBuffer,
          offsetVals,
          _offsetType,
          _componentCount);
    }

    const size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _arrayOffsetBuffer, _offsetType);
    const size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _arrayOffsetBuffer,
        _offsetType);
    const gsl::span<const std::byte> offsetVals(
        _stringOffsetBuffer.data() + currentOffset,
        (nextOffset - currentOffset + _offsetSize));
    return MetadataArrayView<std::string_view>(
        _valueBuffer,
        offsetVals,
        _offsetType,
        (nextOffset - currentOffset) / _offsetSize);
  }

  MetadataArrayView<bool> getBooleanArray(int64_t instance) const noexcept {
    if (_componentCount > 0) {
      const size_t offsetBits = _componentCount * instance;
      const size_t nextOffsetBits = _componentCount * (instance + 1);
      const gsl::span<const std::byte> buffer(
          _valueBuffer.data() + offsetBits / 8,
          (nextOffsetBits / 8 - offsetBits / 8 + 1));
      return MetadataArrayView<bool>(buffer, offsetBits % 8, _componentCount);
    }

    const size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _arrayOffsetBuffer, _offsetType);
    const size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _arrayOffsetBuffer,
        _offsetType);

    const size_t totalBits = nextOffset - currentOffset;
    const gsl::span<const std::byte> buffer(
        _valueBuffer.data() + currentOffset / 8,
        (nextOffset / 8 - currentOffset / 8 + 1));
    return MetadataArrayView<bool>(buffer, currentOffset % 8, totalBits);
  }

  static int64_t getOffsetSize(PropertyType offsetType) noexcept {
    switch (offsetType) {
    case CesiumGltf::PropertyType::Uint8:
      return sizeof(uint8_t);
    case CesiumGltf::PropertyType::Uint16:
      return sizeof(uint16_t);
    case CesiumGltf::PropertyType::Uint32:
      return sizeof(uint32_t);
    case CesiumGltf::PropertyType::Uint64:
      return sizeof(uint64_t);
    default:
      return 0;
    }
  }

  static size_t getOffsetFromOffsetBuffer(
      size_t instance,
      const gsl::span<const std::byte>& offsetBuffer,
      PropertyType offsetType) noexcept {
    switch (offsetType) {
    case PropertyType::Uint8: {
      assert(instance < offsetBuffer.size() / sizeof(uint8_t));
      const uint8_t offset = *reinterpret_cast<const uint8_t*>(
          offsetBuffer.data() + instance * sizeof(uint8_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint16: {
      assert(instance < offsetBuffer.size() / sizeof(uint16_t));
      const uint16_t offset = *reinterpret_cast<const uint16_t*>(
          offsetBuffer.data() + instance * sizeof(uint16_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint32: {
      assert(instance < offsetBuffer.size() / sizeof(uint32_t));
      const uint32_t offset = *reinterpret_cast<const uint32_t*>(
          offsetBuffer.data() + instance * sizeof(uint32_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint64: {
      assert(instance < offsetBuffer.size() / sizeof(uint64_t));
      const uint64_t offset = *reinterpret_cast<const uint64_t*>(
          offsetBuffer.data() + instance * sizeof(uint64_t));
      return static_cast<size_t>(offset);
    }
    default:
      assert(false && "Offset type has unknown type");
      return 0;
    }
  }

  MetadataPropertyViewStatus _status;
  gsl::span<const std::byte> _valueBuffer;
  gsl::span<const std::byte> _arrayOffsetBuffer;
  gsl::span<const std::byte> _stringOffsetBuffer;
  PropertyType _offsetType;
  int64_t _offsetSize;
  int64_t _componentCount;
  int64_t _instanceCount;
  bool _normalized;
};
} // namespace CesiumGltf
