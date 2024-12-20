#pragma once

#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyTransformations.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumGltf/PropertyView.h>
#include <CesiumUtility/Assert.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace CesiumGltf {
/**
 * @brief Indicates the status of a property table property view.
 *
 * The {@link PropertyTablePropertyView} constructor always completes successfully.
 * However, it may not always reflect the actual content of the
 * {@link PropertyTableProperty}, but instead indicate that its `size` is 0.
 * This enumeration provides the reason.
 */
class PropertyTablePropertyViewStatus : public PropertyViewStatus {
public:
  /**
   * @brief This property view was initialized from an invalid
   * {@link PropertyTable}.
   */
  static const PropertyViewStatusType ErrorInvalidPropertyTable = 14;

  /**
   * @brief This property view does not have a valid value buffer view index.
   */
  static const PropertyViewStatusType ErrorInvalidValueBufferView = 15;

  /**
   * @brief This array property view does not have a valid array offset buffer
   * view index.
   */
  static const PropertyViewStatusType ErrorInvalidArrayOffsetBufferView = 16;

  /**
   * @brief This string property view does not have a valid string offset buffer
   * view index.
   */
  static const PropertyViewStatusType ErrorInvalidStringOffsetBufferView = 17;

  /**
   * @brief This property view has a valid value buffer view, but the buffer
   * view specifies an invalid buffer index.
   */
  static const PropertyViewStatusType ErrorInvalidValueBuffer = 18;

  /**
   * @brief This property view has a valid array string buffer view, but the
   * buffer view specifies an invalid buffer index.
   */
  static const PropertyViewStatusType ErrorInvalidArrayOffsetBuffer = 19;

  /**
   * @brief This property view has a valid string offset buffer view, but the
   * buffer view specifies an invalid buffer index.
   */
  static const PropertyViewStatusType ErrorInvalidStringOffsetBuffer = 20;

  /**
   * @brief This property view has a buffer view that points outside the bounds
   * of its target buffer.
   */
  static const PropertyViewStatusType ErrorBufferViewOutOfBounds = 21;

  /**
   * @brief This property view has an invalid buffer view; its length is not
   * a multiple of the size of its type / offset type.
   */
  static const PropertyViewStatusType
      ErrorBufferViewSizeNotDivisibleByTypeSize = 22;

  /**
   * @brief This property view has an invalid buffer view; its length does not
   * match the size of the property table.
   */
  static const PropertyViewStatusType
      ErrorBufferViewSizeDoesNotMatchPropertyTableCount = 23;

  /**
   * @brief This array property view has both a fixed length and an offset
   * buffer view defined.
   */
  static const PropertyViewStatusType ErrorArrayCountAndOffsetBufferCoexist =
      24;

  /**
   * @brief This array property view has neither a fixed length nor an offset
   * buffer view defined.
   */
  static const PropertyViewStatusType ErrorArrayCountAndOffsetBufferDontExist =
      25;

  /**
   * @brief This property view has an unknown array offset type.
   */
  static const PropertyViewStatusType ErrorInvalidArrayOffsetType = 26;

  /**
   * @brief This property view has an unknown string offset type.
   */
  static const PropertyViewStatusType ErrorInvalidStringOffsetType = 27;

  /**
   * @brief This property view's array offset values are not sorted in ascending
   * order.
   */
  static const PropertyViewStatusType ErrorArrayOffsetsNotSorted = 28;

  /**
   * @brief This property view's string offset values are not sorted in
   * ascending order.
   */
  static const PropertyViewStatusType ErrorStringOffsetsNotSorted = 29;

  /**
   * @brief This property view has an array offset that is out of bounds.
   */
  static const PropertyViewStatusType ErrorArrayOffsetOutOfBounds = 30;

  /**
   * @brief This property view has a string offset that is out of bounds.
   */
  static const PropertyViewStatusType ErrorStringOffsetOutOfBounds = 31;
};

/**
 * @brief Returns the size in bytes of a \ref PropertyComponentType used as the
 * `arrayOffsetType` in the constructor of \ref PropertyTablePropertyView.
 */
int64_t getOffsetTypeSize(PropertyComponentType offsetType) noexcept;

/**
 * @brief A view on the data of the {@link PropertyTableProperty} that is created
 * by a {@link PropertyTableView}.
 */
template <typename ElementType, bool Normalized = false>
class PropertyTablePropertyView;

/**
 * @brief A view on the data of the {@link PropertyTableProperty} that is created
 * by a {@link PropertyTableView}.
 *
 * It provides utility to retrieve the actual data stored in the
 * {@link PropertyTableProperty::values} like an array of elements. Data of each
 * instance can be accessed through the {@link PropertyTablePropertyView<ElementType, false>::get} method.
 *
 * @param ElementType must be one of the following: a scalar (uint8_t, int8_t,
 * uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, float, double), a
 * glm vecN composed of one of the scalar types, a glm matN composed of one of
 * the scalar types, bool, std::string_view, or PropertyArrayView<T> with T as
 * one of the aforementioned types.
 */
template <typename ElementType>
class PropertyTablePropertyView<ElementType, false>
    : public PropertyView<ElementType, false> {
public:
  /**
   * @brief Constructs an invalid instance for a non-existent property.
   */
  PropertyTablePropertyView()
      : PropertyView<ElementType, false>(),
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
   * @param status The code from {@link PropertyTablePropertyViewStatus} indicating the error with the property.
   */
  PropertyTablePropertyView(PropertyViewStatusType status)
      : PropertyView<ElementType, false>(status),
        _values{},
        _size{0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0},
        _stringOffsets{},
        _stringOffsetType{PropertyComponentType::None},
        _stringOffsetTypeSize{0} {
    CESIUM_ASSERT(
        this->_status != PropertyTablePropertyViewStatus::Valid &&
        "An empty property view should not be constructed with a valid status");
  }

  /**
   * @brief Constructs an instance of an empty property that specifies a default
   * value. Although this property has no data, it can return the default value
   * when {@link PropertyTablePropertyView<ElementType, false>::get} is called. However,
   * {@link PropertyTablePropertyView<ElementType, false>::getRaw} cannot be used.
   *
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   */
  PropertyTablePropertyView(const ClassProperty& classProperty, int64_t size)
      : PropertyView<ElementType, false>(classProperty),
        _values{},
        _size{0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0},
        _stringOffsets{},
        _stringOffsetType{PropertyComponentType::None},
        _stringOffsetTypeSize{0} {
    if (this->_status != PropertyTablePropertyViewStatus::Valid) {
      // Don't override the status / size if something is wrong with the class
      // property's definition.
      return;
    }

    if (!classProperty.defaultProperty) {
      // This constructor should only be called if the class property *has* a
      // default value. But in the case that it does not, this property view
      // becomes invalid.
      this->_status = PropertyTablePropertyViewStatus::ErrorNonexistentProperty;
      return;
    }

    this->_status = PropertyTablePropertyViewStatus::EmptyPropertyWithDefault;
    this->_size = size;
  }

  /**
   * @brief Construct an instance pointing to data specified by a {@link PropertyTableProperty}.
   * Used for non-array or fixed-length array data.
   *
   * @param property The {@link PropertyTableProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   * @param values The raw buffer specified by {@link PropertyTableProperty::values}
   */
  PropertyTablePropertyView(
      const PropertyTableProperty& property,
      const ClassProperty& classProperty,
      int64_t size,
      std::span<const std::byte> values) noexcept
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
   * @param property The {@link PropertyTableProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   * @param values The raw buffer specified by {@link PropertyTableProperty::values}
   * @param arrayOffsets The raw buffer specified by {@link PropertyTableProperty::arrayOffsets}
   * @param stringOffsets The raw buffer specified by {@link PropertyTableProperty::stringOffsets}
   * @param arrayOffsetType The offset type of arrayOffsets specified by {@link PropertyTableProperty::arrayOffsetType}
   * @param stringOffsetType The offset type of stringOffsets specified by {@link PropertyTableProperty::stringOffsetType}
   */
  PropertyTablePropertyView(
      const PropertyTableProperty& property,
      const ClassProperty& classProperty,
      int64_t size,
      std::span<const std::byte> values,
      std::span<const std::byte> arrayOffsets,
      std::span<const std::byte> stringOffsets,
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
   * @brief Get the value of an element in the {@link PropertyTable},
   * with all value transforms applied. That is, if the property specifies an
   * offset and scale, they will be applied to the value before the value is
   * returned.
   *
   * If this property has a specified "no data" value, and the retrieved element
   * is equal to that value, then this will return the property's specified
   * default value. If the property did not provide a default value, this
   * returns std::nullopt.
   *
   * @param index The element index
   * @return The value of the element, or std::nullopt if it matches the "no
   * data" value
   */
  std::optional<PropertyValueViewToCopy<ElementType>>
  get(int64_t index) const noexcept {
    if (this->_status ==
        PropertyTablePropertyViewStatus::EmptyPropertyWithDefault) {
      CESIUM_ASSERT(index >= 0 && "index must be non-negative");
      CESIUM_ASSERT(index < size() && "index must be less than size");

      return propertyValueViewToCopy(this->defaultValue());
    }

    ElementType value = getRaw(index);

    if (value == this->noData()) {
      return propertyValueViewToCopy(this->defaultValue());
    } else if constexpr (IsMetadataNumeric<ElementType>::value) {
      return transformValue(value, this->offset(), this->scale());
    } else if constexpr (IsMetadataNumericArray<ElementType>::value) {
      return transformArray(value, this->offset(), this->scale());
    } else {
      return value;
    }
  }

  /**
   * @brief Get the raw value of an element of the {@link PropertyTable},
   * without offset or scale applied.
   *
   * If this property has a specified "no data" value, the raw value will still
   * be returned, even if it equals the "no data" value.
   *
   * @param index The element index
   * @return The value of the element
   */
  ElementType getRaw(int64_t index) const noexcept {
    CESIUM_ASSERT(
        this->_status == PropertyTablePropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");
    CESIUM_ASSERT(
        size() > 0 &&
        "Check the size() of the view to make sure it's not empty");
    CESIUM_ASSERT(index >= 0 && "index must be non-negative");
    CESIUM_ASSERT(index < size() && "index must be less than size");

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
  int64_t size() const noexcept { return _size; }

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
      const std::span<const std::byte> values(
          _values.data() + index * arraySize,
          arraySize);
      return PropertyArrayView<T>{values};
    }

    // Handle variable-length arrays. The offsets are interpreted as array
    // indices, not byte offsets, so they must be multiplied by sizeof(T)
    const size_t currentOffset =
        getOffsetFromOffsetsBuffer(index, _arrayOffsets, _arrayOffsetType) *
        sizeof(T);
    const size_t nextOffset =
        getOffsetFromOffsetsBuffer(index + 1, _arrayOffsets, _arrayOffsetType) *
        sizeof(T);
    const std::span<const std::byte> values(
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
      const std::span<const std::byte> stringOffsetValues(
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
    const std::span<const std::byte> stringOffsetValues(
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
      const std::span<const std::byte> buffer(
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
    const std::span<const std::byte> buffer(
        _values.data() + currentOffset / 8,
        (nextOffset / 8 - currentOffset / 8 + 1));
    return PropertyArrayView<bool>(buffer, currentOffset % 8, totalBits);
  }

  std::span<const std::byte> _values;
  int64_t _size;

  std::span<const std::byte> _arrayOffsets;
  PropertyComponentType _arrayOffsetType;
  int64_t _arrayOffsetTypeSize;

  std::span<const std::byte> _stringOffsets;
  PropertyComponentType _stringOffsetType;
  int64_t _stringOffsetTypeSize;
};

/**
 * @brief A view on the normalized data of the {@link PropertyTableProperty}
 * that is created by a {@link PropertyTableView}.
 *
 * It provides utility to retrieve the actual data stored in the
 * {@link PropertyTableProperty::values} like an array of elements. Data of each
 * instance can be accessed through the \ref get method.
 *
 * @param ElementType must be one of the following: an integer scalar (uint8_t,
 * int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t), a glm vecN
 * composed of one of the integer scalar types, a glm matN composed of one of
 * the integer scalar types, or PropertyArrayView<T> with T as one of the
 * aforementioned types.
 */
template <typename ElementType>
class PropertyTablePropertyView<ElementType, true>
    : public PropertyView<ElementType, true> {
private:
  using NormalizedType = typename TypeToNormalizedType<ElementType>::type;

public:
  /**
   * @brief Constructs an invalid instance for a non-existent property.
   */
  PropertyTablePropertyView()
      : PropertyView<ElementType, true>(),
        _values{},
        _size{0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0} {}

  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyTablePropertyViewStatus} indicating the error with the property.
   */
  PropertyTablePropertyView(PropertyViewStatusType status)
      : PropertyView<ElementType, true>(status),
        _values{},
        _size{0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0} {
    CESIUM_ASSERT(
        this->_status != PropertyTablePropertyViewStatus::Valid &&
        "An empty property view should not be constructed with a valid status");
  }

  /**
   * @brief Constructs an instance of an empty property that specifies a default
   * value. Although this property has no data, it can return the default value
   * when {@link PropertyTablePropertyView<ElementType, true>::get} is called. However,
   * {@link PropertyTablePropertyView<ElementType, true>::getRaw} cannot be used.
   *
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   */
  PropertyTablePropertyView(const ClassProperty& classProperty, int64_t size)
      : PropertyView<ElementType, true>(classProperty),
        _values{},
        _size{0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0} {
    if (this->_status != PropertyTablePropertyViewStatus::Valid) {
      // Don't override the status / size if something is wrong with the class
      // property's definition.
      return;
    }

    if (!classProperty.defaultProperty) {
      // This constructor should only be called if the class property *has* a
      // default value. But in the case that it does not, this property view
      // becomes invalid.
      this->_status = PropertyTablePropertyViewStatus::ErrorNonexistentProperty;
      return;
    }

    this->_status = PropertyTablePropertyViewStatus::EmptyPropertyWithDefault;
    this->_size = size;
  }

  /**
   * @brief Construct an instance pointing to data specified by a {@link PropertyTableProperty}.
   * Used for non-array or fixed-length array data.
   *
   * @param property The {@link PropertyTableProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   * @param values The raw buffer specified by {@link PropertyTableProperty::values}
   */
  PropertyTablePropertyView(
      const PropertyTableProperty& property,
      const ClassProperty& classProperty,
      int64_t size,
      std::span<const std::byte> values) noexcept
      : PropertyView<ElementType, true>(classProperty, property),
        _values{values},
        _size{
            this->_status == PropertyTablePropertyViewStatus::Valid ? size : 0},
        _arrayOffsets{},
        _arrayOffsetType{PropertyComponentType::None},
        _arrayOffsetTypeSize{0} {}

  /**
   * @brief Construct an instance pointing to the data specified by a {@link PropertyTableProperty}.
   *
   *
   * @param property The {@link PropertyTableProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   * @param values The raw buffer specified by {@link PropertyTableProperty::values}
   * @param arrayOffsets The raw buffer specified by {@link PropertyTableProperty::arrayOffsets}
   * @param arrayOffsetType The offset type of arrayOffsets specified by {@link PropertyTableProperty::arrayOffsetType}
   */
  PropertyTablePropertyView(
      const PropertyTableProperty& property,
      const ClassProperty& classProperty,
      int64_t size,
      std::span<const std::byte> values,
      std::span<const std::byte> arrayOffsets,
      PropertyComponentType arrayOffsetType) noexcept
      : PropertyView<ElementType, true>(classProperty, property),
        _values{values},
        _size{
            this->_status == PropertyTablePropertyViewStatus::Valid ? size : 0},
        _arrayOffsets{arrayOffsets},
        _arrayOffsetType{arrayOffsetType},
        _arrayOffsetTypeSize{getOffsetTypeSize(arrayOffsetType)} {}

  /**
   * @brief Get the value of an element of the {@link PropertyTable},
   * with normalization and other value transforms applied. In other words, the
   * value will be normalized, then transformed by the property's offset
   * and scale, if they are defined.
   *
   * If this property has a specified "no data" value, and the retrieved element
   * is equal to that value, then this will return the property's specified
   * default value. If the property did not provide a default value, this
   * returns std::nullopt.
   *
   * @param index The element index
   * @return The value of the element, or std::nullopt if it matches the "no
   * data" value
   */
  std::optional<PropertyValueViewToCopy<NormalizedType>>
  get(int64_t index) const noexcept {
    if (this->_status ==
        PropertyTablePropertyViewStatus::EmptyPropertyWithDefault) {
      CESIUM_ASSERT(index >= 0 && "index must be non-negative");
      CESIUM_ASSERT(index < size() && "index must be less than size");

      return propertyValueViewToCopy(this->defaultValue());
    }

    ElementType value = getRaw(index);
    if (this->noData() && value == *(this->noData())) {
      return propertyValueViewToCopy(this->defaultValue());
    } else if constexpr (IsMetadataScalar<ElementType>::value) {
      return transformValue<NormalizedType>(
          normalize<ElementType>(value),
          this->offset(),
          this->scale());
    } else if constexpr (IsMetadataVecN<ElementType>::value) {
      constexpr glm::length_t N = ElementType::length();
      using T = typename ElementType::value_type;
      using NormalizedT = typename NormalizedType::value_type;
      return transformValue<glm::vec<N, NormalizedT>>(
          normalize<N, T>(value),
          this->offset(),
          this->scale());
    } else if constexpr (IsMetadataMatN<ElementType>::value) {
      constexpr glm::length_t N = ElementType::length();
      using T = typename ElementType::value_type;
      using NormalizedT = typename NormalizedType::value_type;
      return transformValue<glm::mat<N, N, NormalizedT>>(
          normalize<N, T>(value),
          this->offset(),
          this->scale());
    } else if constexpr (IsMetadataArray<ElementType>::value) {
      using ArrayElementType = typename MetadataArrayType<ElementType>::type;
      if constexpr (IsMetadataScalar<ArrayElementType>::value) {
        return transformNormalizedArray<ArrayElementType>(
            value,
            this->offset(),
            this->scale());
      } else if constexpr (IsMetadataVecN<ArrayElementType>::value) {
        constexpr glm::length_t N = ArrayElementType::length();
        using T = typename ArrayElementType::value_type;
        return transformNormalizedVecNArray<N, T>(
            value,
            this->offset(),
            this->scale());
      } else if constexpr (IsMetadataMatN<ArrayElementType>::value) {
        constexpr glm::length_t N = ArrayElementType::length();
        using T = typename ArrayElementType::value_type;
        return transformNormalizedMatNArray<N, T>(
            value,
            this->offset(),
            this->scale());
      }
    }
  }

  /**
   * @brief Get the raw value of an element of the {@link PropertyTable},
   * without offset, scale, or normalization applied.
   *
   * If this property has a specified "no data" value, the raw value will still
   * be returned, even if it equals the "no data" value.
   *
   * @param index The element index
   * @return The value of the element
   */
  ElementType getRaw(int64_t index) const noexcept {
    CESIUM_ASSERT(
        this->_status == PropertyTablePropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");
    CESIUM_ASSERT(
        size() > 0 &&
        "Check the size() of the view to make sure it's not empty");
    CESIUM_ASSERT(index >= 0 && "index must be non-negative");
    CESIUM_ASSERT(index < size() && "index must be less than size");

    if constexpr (IsMetadataNumeric<ElementType>::value) {
      return getValue(index);
    }

    if constexpr (IsMetadataNumericArray<ElementType>::value) {
      return getArrayValues<typename MetadataArrayType<ElementType>::type>(
          index);
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
  ElementType getValue(int64_t index) const noexcept {
    return reinterpret_cast<const ElementType*>(_values.data())[index];
  }

  template <typename T>
  PropertyArrayView<T> getArrayValues(int64_t index) const noexcept {
    size_t count = static_cast<size_t>(this->arrayCount());
    // Handle fixed-length arrays
    if (count > 0) {
      size_t arraySize = count * sizeof(T);
      const std::span<const std::byte> values(
          _values.data() + index * arraySize,
          arraySize);
      return PropertyArrayView<T>{values};
    }

    // Handle variable-length arrays. The offsets are interpreted as array
    // indices, not byte offsets, so they must be multiplied by sizeof(T)
    const size_t currentOffset =
        getOffsetFromOffsetsBuffer(index, _arrayOffsets, _arrayOffsetType) *
        sizeof(T);
    const size_t nextOffset =
        getOffsetFromOffsetsBuffer(index + 1, _arrayOffsets, _arrayOffsetType) *
        sizeof(T);
    const std::span<const std::byte> values(
        _values.data() + currentOffset,
        nextOffset - currentOffset);
    return PropertyArrayView<T>{values};
  }

  std::span<const std::byte> _values;
  int64_t _size;

  std::span<const std::byte> _arrayOffsets;
  PropertyComponentType _arrayOffsetType;
  int64_t _arrayOffsetTypeSize;
};

} // namespace CesiumGltf
