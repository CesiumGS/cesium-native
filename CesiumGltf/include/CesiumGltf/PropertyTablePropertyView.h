#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyTransformations.h"
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
 * However, it may not always reflect the actual content of the
 * {@link PropertyTableProperty}, but instead indicate that its
 * {@link PropertyTablePropertyView::size} is 0.
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
 * instance can be accessed through the {@link PropertyTablePropertyView::get} method.
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
  PropertyTablePropertyView();

  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The code from {@link PropertyTablePropertyViewStatus} indicating the error with the property.
   */
  PropertyTablePropertyView(PropertyViewStatusType status);

  /**
   * @brief Constructs an instance of an empty property that specifies a default
   * value. Although this property has no data, it can return the default value
   * when {@link PropertyTablePropertyView::get} is called. However,
   * {@link PropertyTablePropertyView::getRaw} cannot be used.
   *
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   */
  PropertyTablePropertyView(const ClassProperty& classProperty, int64_t size);

  /**
   * @brief Construct an instance pointing to data specified by a {@link PropertyTableProperty}.
   * Used for non-array or fixed-length array data.
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
      gsl::span<const std::byte> values) noexcept;

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
      gsl::span<const std::byte> values,
      gsl::span<const std::byte> arrayOffsets,
      gsl::span<const std::byte> stringOffsets,
      PropertyComponentType arrayOffsetType,
      PropertyComponentType stringOffsetType) noexcept;

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
  std::optional<ElementType> get(int64_t index) const noexcept;

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
  ElementType getRaw(int64_t index) const noexcept;

  /**
   * @brief Get the number of elements in this
   * PropertyTablePropertyView. If the view is valid, this returns
   * {@link PropertyTable::count}. Otherwise, this returns 0.
   *
   * @return The number of elements in this PropertyTablePropertyView.
   */
  int64_t size() const noexcept;

private:
  ElementType getNumericValue(int64_t index) const noexcept;

  bool getBooleanValue(int64_t index) const noexcept;

  std::string_view getStringValue(int64_t index) const noexcept;

  template <typename T>
  PropertyArrayView<T> getNumericArrayValues(int64_t index) const noexcept;

  PropertyArrayView<std::string_view>
  getStringArrayValues(int64_t index) const noexcept;

  PropertyArrayView<bool> getBooleanArrayValues(int64_t index) const noexcept;

  gsl::span<const std::byte> _values;
  int64_t _size;

  gsl::span<const std::byte> _arrayOffsets;
  PropertyComponentType _arrayOffsetType;
  int64_t _arrayOffsetTypeSize;

  gsl::span<const std::byte> _stringOffsets;
  PropertyComponentType _stringOffsetType;
  int64_t _stringOffsetTypeSize;
};

/**
 * @brief A view on the normalized data of the {@link PropertyTableProperty}
 * that is created by a {@link PropertyTableView}.
 *
 * It provides utility to retrieve the actual data stored in the
 * {@link PropertyTableProperty::values} like an array of elements. Data of each
 * instance can be accessed through the {@link PropertyTablePropertyView::get} method.
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
  PropertyTablePropertyView();

  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyTablePropertyViewStatus} indicating the error with the property.
   */
  PropertyTablePropertyView(PropertyViewStatusType status);

  /**
   * @brief Constructs an instance of an empty property that specifies a default
   * value. Although this property has no data, it can return the default value
   * when {@link PropertyTablePropertyView::get} is called. However,
   * {@link PropertyTablePropertyView::getRaw} cannot be used.
   *
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   */
  PropertyTablePropertyView(const ClassProperty& classProperty, int64_t size);

  /**
   * @brief Construct an instance pointing to data specified by a {@link PropertyTableProperty}.
   * Used for non-array or fixed-length array data.
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
      gsl::span<const std::byte> values) noexcept;

  /**
   * @brief Construct an instance pointing to the data specified by a {@link PropertyTableProperty}.
   *
   *
   * @param property The {@link PropertyTableProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param size The number of elements in the property table specified by {@link PropertyTable::count}
   * @param values The raw buffer specified by {@link PropertyTableProperty::values}
   * @param arrayOffsets The raw buffer specified by {@link PropertyTableProperty::arrayOffsets}
   * @param offsetType The offset type of arrayOffsets specified by {@link PropertyTableProperty::arrayOffsetType}
   */
  PropertyTablePropertyView(
      const PropertyTableProperty& property,
      const ClassProperty& classProperty,
      int64_t size,
      gsl::span<const std::byte> values,
      gsl::span<const std::byte> arrayOffsets,
      PropertyComponentType arrayOffsetType) noexcept;

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
  std::optional<NormalizedType> get(int64_t index) const noexcept;

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
  ElementType getRaw(int64_t index) const noexcept;

  /**
   * @brief Get the number of elements in this
   * PropertyTablePropertyView. If the view is valid, this returns
   * {@link PropertyTable::count}. Otherwise, this returns 0.
   *
   * @return The number of elements in this PropertyTablePropertyView.
   */
  int64_t size() const noexcept;

private:
  ElementType getValue(int64_t index) const noexcept;

  template <typename T>
  PropertyArrayView<T> getArrayValues(int64_t index) const noexcept;

  gsl::span<const std::byte> _values;
  int64_t _size;

  gsl::span<const std::byte> _arrayOffsets;
  PropertyComponentType _arrayOffsetType;
  int64_t _arrayOffsetTypeSize;
};

extern template class PropertyTablePropertyView<int8_t, false>;
extern template class PropertyTablePropertyView<uint8_t, false>;
extern template class PropertyTablePropertyView<int16_t, false>;
extern template class PropertyTablePropertyView<uint16_t, false>;
extern template class PropertyTablePropertyView<int32_t, false>;
extern template class PropertyTablePropertyView<uint32_t, false>;
extern template class PropertyTablePropertyView<int64_t, false>;
extern template class PropertyTablePropertyView<uint64_t, false>;
extern template class PropertyTablePropertyView<float>;
extern template class PropertyTablePropertyView<double>;
extern template class PropertyTablePropertyView<bool>;
extern template class PropertyTablePropertyView<std::string_view>;
extern template class PropertyTablePropertyView<glm::vec<2, int8_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<2, uint8_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<2, int16_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<2, uint16_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<2, int32_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<2, uint32_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<2, int64_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<2, uint64_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<2, float>>;
extern template class PropertyTablePropertyView<glm::vec<2, double>>;
extern template class PropertyTablePropertyView<glm::vec<3, int8_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<3, uint8_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<3, int16_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<3, uint16_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<3, int32_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<3, uint32_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<3, int64_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<3, uint64_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<3, float>>;
extern template class PropertyTablePropertyView<glm::vec<3, double>>;
extern template class PropertyTablePropertyView<glm::vec<4, int8_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<4, uint8_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<4, int16_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<4, uint16_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<4, int32_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<4, uint32_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<4, int64_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<4, uint64_t>, false>;
extern template class PropertyTablePropertyView<glm::vec<4, float>>;
extern template class PropertyTablePropertyView<glm::vec<4, double>>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, int8_t>, false>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, uint8_t>, false>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, int16_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<2, 2, uint16_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, int32_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<2, 2, uint32_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, int64_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<2, 2, uint64_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, float>>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, double>>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, int8_t>, false>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, uint8_t>, false>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, int16_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<3, 3, uint16_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, int32_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<3, 3, uint32_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, int64_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<3, 3, uint64_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, float>>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, double>>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, int8_t>, false>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, uint8_t>, false>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, int16_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<4, 4, uint16_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, int32_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<4, 4, uint32_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, int64_t>, false>;
extern template class PropertyTablePropertyView<
    glm::mat<4, 4, uint64_t>,
    false>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, float>>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, double>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<int8_t>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<uint8_t>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<int16_t>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<uint16_t>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<int32_t>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<uint32_t>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<int64_t>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<uint64_t>,
    false>;
extern template class PropertyTablePropertyView<PropertyArrayView<float>>;
extern template class PropertyTablePropertyView<PropertyArrayView<double>>;
extern template class PropertyTablePropertyView<PropertyArrayView<bool>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<std::string_view>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, float>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, double>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, float>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, double>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, float>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, double>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, float>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, double>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, float>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, double>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint8_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint16_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint32_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint64_t>>,
    false>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, float>>>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, double>>>;

extern template class PropertyTablePropertyView<int8_t, true>;
extern template class PropertyTablePropertyView<uint8_t, true>;
extern template class PropertyTablePropertyView<int16_t, true>;
extern template class PropertyTablePropertyView<uint16_t, true>;
extern template class PropertyTablePropertyView<int32_t, true>;
extern template class PropertyTablePropertyView<uint32_t, true>;
extern template class PropertyTablePropertyView<int64_t, true>;
extern template class PropertyTablePropertyView<uint64_t, true>;
extern template class PropertyTablePropertyView<glm::vec<2, int8_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<2, uint8_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<2, int16_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<2, uint16_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<2, int32_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<2, uint32_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<2, int64_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<2, uint64_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<3, int8_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<3, uint8_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<3, int16_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<3, uint16_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<3, int32_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<3, uint32_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<3, int64_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<3, uint64_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<4, int8_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<4, uint8_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<4, int16_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<4, uint16_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<4, int32_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<4, uint32_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<4, int64_t>, true>;
extern template class PropertyTablePropertyView<glm::vec<4, uint64_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, int8_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, uint8_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, int16_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, uint16_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, int32_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, uint32_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, int64_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<2, 2, uint64_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, int8_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, uint8_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, int16_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, uint16_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, int32_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, uint32_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, int64_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<3, 3, uint64_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, int8_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, uint8_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, int16_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, uint16_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, int32_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, uint32_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, int64_t>, true>;
extern template class PropertyTablePropertyView<glm::mat<4, 4, uint64_t>, true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<int8_t>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<uint8_t>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<int16_t>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<uint16_t>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<int32_t>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<uint32_t>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<int64_t>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<uint64_t>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint8_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint16_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint32_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int64_t>>,
    true>;
extern template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint64_t>>,
    true>;

} // namespace CesiumGltf
