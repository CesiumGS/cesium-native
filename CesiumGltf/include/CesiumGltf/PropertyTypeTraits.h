#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyType.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <type_traits>

namespace CesiumGltf {
/**
 * @brief Check if a C++ type can be represented as a scalar property type
 */
template <typename... T> struct IsMetadataScalar;
template <typename T> struct IsMetadataScalar<T> : std::false_type {};
template <> struct IsMetadataScalar<uint8_t> : std::true_type {};
template <> struct IsMetadataScalar<int8_t> : std::true_type {};
template <> struct IsMetadataScalar<uint16_t> : std::true_type {};
template <> struct IsMetadataScalar<int16_t> : std::true_type {};
template <> struct IsMetadataScalar<uint32_t> : std::true_type {};
template <> struct IsMetadataScalar<int32_t> : std::true_type {};
template <> struct IsMetadataScalar<uint64_t> : std::true_type {};
template <> struct IsMetadataScalar<int64_t> : std::true_type {};
template <> struct IsMetadataScalar<float> : std::true_type {};
template <> struct IsMetadataScalar<double> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an integer property type
 */
template <typename... T> struct IsMetadataInteger;
template <typename T> struct IsMetadataInteger<T> : std::false_type {};
template <> struct IsMetadataInteger<uint8_t> : std::true_type {};
template <> struct IsMetadataInteger<int8_t> : std::true_type {};
template <> struct IsMetadataInteger<uint16_t> : std::true_type {};
template <> struct IsMetadataInteger<int16_t> : std::true_type {};
template <> struct IsMetadataInteger<uint32_t> : std::true_type {};
template <> struct IsMetadataInteger<int32_t> : std::true_type {};
template <> struct IsMetadataInteger<uint64_t> : std::true_type {};
template <> struct IsMetadataInteger<int64_t> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a floating-point property
 * type.
 */
template <typename... T> struct IsMetadataFloating;
template <typename T> struct IsMetadataFloating<T> : std::false_type {};
template <> struct IsMetadataFloating<float> : std::true_type {};
template <> struct IsMetadataFloating<double> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a vecN type.
 */
template <typename... T> struct IsMetadataVecN;
template <typename T> struct IsMetadataVecN<T> : std::false_type {};
template <glm::length_t n, typename T, glm::qualifier P>
struct IsMetadataVecN<glm::vec<n, T, P>> : IsMetadataScalar<T> {};

/**
 * @brief Check if a C++ type can be represented as a matN type.
 */
template <typename... T> struct IsMetadataMatN;
template <typename T> struct IsMetadataMatN<T> : std::false_type {};
template <glm::length_t n, typename T, glm::qualifier P>
struct IsMetadataMatN<glm::mat<n, n, T, P>> : IsMetadataScalar<T> {};

/**
 * @brief Check if a C++ type can be represented as a numeric property, i.e.
 * a scalar / vecN / matN type.
 */
template <typename... T> struct IsMetadataNumeric;
template <typename T> struct IsMetadataNumeric<T> {
  static constexpr bool value = IsMetadataScalar<T>::value ||
                                IsMetadataVecN<T>::value ||
                                IsMetadataMatN<T>::value;
};

/**
 * @brief Check if a C++ type can be represented as a boolean property type
 */
template <typename... T> struct IsMetadataBoolean;
template <typename T> struct IsMetadataBoolean<T> : std::false_type {};
template <> struct IsMetadataBoolean<bool> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a string property type
 */
template <typename... T> struct IsMetadataString;
template <typename T> struct IsMetadataString<T> : std::false_type {};
template <> struct IsMetadataString<std::string_view> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array.
 */
template <typename... T> struct IsMetadataArray;
template <typename T> struct IsMetadataArray<T> : std::false_type {};
template <typename T>
struct IsMetadataArray<PropertyArrayView<T>> : std::true_type {};
template <typename T>
struct IsMetadataArray<PropertyArrayCopy<T>> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array of numeric elements
 * property type
 */
template <typename... T> struct IsMetadataNumericArray;
template <typename T> struct IsMetadataNumericArray<T> : std::false_type {};
template <typename T> struct IsMetadataNumericArray<PropertyArrayView<T>> {
  static constexpr bool value = IsMetadataNumeric<T>::value;
};
template <typename T> struct IsMetadataNumericArray<PropertyArrayCopy<T>> {
  static constexpr bool value = IsMetadataNumeric<T>::value;
};

/**
 * @brief Check if a C++ type can be represented as an array of booleans
 * property type
 */
template <typename... T> struct IsMetadataBooleanArray;
template <typename T> struct IsMetadataBooleanArray<T> : std::false_type {};
template <>
struct IsMetadataBooleanArray<PropertyArrayView<bool>> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array of strings property
 * type
 */
template <typename... T> struct IsMetadataStringArray;
template <typename T> struct IsMetadataStringArray<T> : std::false_type {};
template <>
struct IsMetadataStringArray<PropertyArrayView<std::string_view>>
    : std::true_type {};

/**
 * @brief Retrieve the component type of a metadata array
 */
template <typename T> struct MetadataArrayType {
  using type = void;
};
template <typename T>
struct MetadataArrayType<CesiumGltf::PropertyArrayView<T>> {
  using type = T;
};
template <typename T>
struct MetadataArrayType<CesiumGltf::PropertyArrayCopy<T>> {
  using type = T;
};

/**
 * @brief Convert a C++ type to PropertyType and PropertyComponentType
 */
template <typename T> struct TypeToPropertyType;

#pragma region Scalar Property Types

template <> struct TypeToPropertyType<uint8_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<int8_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<uint16_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<int16_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<uint32_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<int32_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<uint64_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<int64_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<float> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<double> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  static constexpr PropertyType value = PropertyType::Scalar;
};
#pragma endregion

#pragma region Vector Property Types

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<2, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<3, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<4, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Vec4;
};

#pragma endregion

#pragma region Matrix Property Types

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<2, 2, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<3, 3, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<4, 4, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Mat4;
};

#pragma endregion

template <> struct TypeToPropertyType<bool> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::None;
  static constexpr PropertyType value = PropertyType::Boolean;
};

template <> struct TypeToPropertyType<std::string_view> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::None;
  static constexpr PropertyType value = PropertyType::String;
};

/**
 * @brief Check if a C++ type can be normalized.
 */
template <typename... T> struct CanBeNormalized;
template <typename T> struct CanBeNormalized<T> : std::false_type {};
template <> struct CanBeNormalized<uint8_t> : std::true_type {};
template <> struct CanBeNormalized<int8_t> : std::true_type {};
template <> struct CanBeNormalized<uint16_t> : std::true_type {};
template <> struct CanBeNormalized<int16_t> : std::true_type {};
template <> struct CanBeNormalized<uint32_t> : std::true_type {};
template <> struct CanBeNormalized<int32_t> : std::true_type {};
template <> struct CanBeNormalized<uint64_t> : std::true_type {};
template <> struct CanBeNormalized<int64_t> : std::true_type {};

template <glm::length_t n, typename T, glm::qualifier P>
struct CanBeNormalized<glm::vec<n, T, P>> : CanBeNormalized<T> {};

template <glm::length_t n, typename T, glm::qualifier P>
struct CanBeNormalized<glm::mat<n, n, T, P>> : CanBeNormalized<T> {};

template <typename T>
struct CanBeNormalized<PropertyArrayView<T>> : CanBeNormalized<T> {};
/**
 * @brief Convert an integer numeric type to the corresponding representation as
 * a double type. Doubles are preferred over floats to maintain more precision.
 */
template <typename T> struct TypeToNormalizedType;

template <> struct TypeToNormalizedType<int8_t> {
  using type = double;
};
template <> struct TypeToNormalizedType<uint8_t> {
  using type = double;
};
template <> struct TypeToNormalizedType<int16_t> {
  using type = double;
};
template <> struct TypeToNormalizedType<uint16_t> {
  using type = double;
};
template <> struct TypeToNormalizedType<int32_t> {
  using type = double;
};
template <> struct TypeToNormalizedType<uint32_t> {
  using type = double;
};
template <> struct TypeToNormalizedType<int64_t> {
  using type = double;
};
template <> struct TypeToNormalizedType<uint64_t> {
  using type = double;
};

template <glm::length_t N, typename T, glm::qualifier Q>
struct TypeToNormalizedType<glm::vec<N, T, Q>> {
  using type = glm::vec<N, double, Q>;
};

template <glm::length_t N, typename T, glm::qualifier Q>
struct TypeToNormalizedType<glm::mat<N, N, T, Q>> {
  using type = glm::mat<N, N, double, Q>;
};

template <> struct TypeToNormalizedType<PropertyArrayView<int8_t>> {
  using type = PropertyArrayView<double>;
};
template <> struct TypeToNormalizedType<PropertyArrayView<uint8_t>> {
  using type = PropertyArrayView<double>;
};
template <> struct TypeToNormalizedType<PropertyArrayView<int16_t>> {
  using type = PropertyArrayView<double>;
};
template <> struct TypeToNormalizedType<PropertyArrayView<uint16_t>> {
  using type = PropertyArrayView<double>;
};
template <> struct TypeToNormalizedType<PropertyArrayView<int32_t>> {
  using type = PropertyArrayView<double>;
};
template <> struct TypeToNormalizedType<PropertyArrayView<uint32_t>> {
  using type = PropertyArrayView<double>;
};
template <> struct TypeToNormalizedType<PropertyArrayView<int64_t>> {
  using type = PropertyArrayView<double>;
};
template <> struct TypeToNormalizedType<PropertyArrayView<uint64_t>> {
  using type = PropertyArrayView<double>;
};

template <glm::length_t N, typename T, glm::qualifier Q>
struct TypeToNormalizedType<PropertyArrayView<glm::vec<N, T, Q>>> {
  using type = PropertyArrayView<glm::vec<N, double, Q>>;
};

template <glm::length_t N, typename T, glm::qualifier Q>
struct TypeToNormalizedType<PropertyArrayView<glm::mat<N, N, T, Q>>> {
  using type = PropertyArrayView<glm::mat<N, N, double, Q>>;
};

/**
 * @brief Transforms a property value type from a view to an equivalent type
 * that owns the data it is viewing. For most property types this is an identity
 * transformation, because most property types are held by value. However, it
 * transforms numeric `PropertyArrayView<T>` to `PropertyArrayCopy<T>` because a
 * `PropertyArrayView<T>` only has a pointer to the value it is viewing.
 *
 * @tparam T The type of the property value view.
 */
template <typename T>
using PropertyValueViewToCopy = std::conditional_t<
    IsMetadataNumericArray<T>::value,
    PropertyArrayCopy<typename MetadataArrayType<T>::type>,
    T>;

template <typename T>
using PropertyValueCopyToView = std::conditional_t<
    IsMetadataNumericArray<T>::value,
    PropertyArrayView<typename MetadataArrayType<T>::type>,
    T>;

/**
 * @brief Creates an optional instance of a type that can be used to own a
 * property value from an optional instance that is only a view on that value.
 * See {@link PropertyValueViewToOwner}.
 *
 * @tparam T
 * @param view
 * @return std::optional<PropertyValueViewToOwner<T>>
 */
template <typename T>
static std::optional<PropertyValueViewToCopy<T>>
propertyValueViewToCopy(const std::optional<T>& view) {
  if constexpr (IsMetadataNumericArray<T>::value) {
    if (view) {
      return std::make_optional<PropertyValueViewToCopy<T>>(
          std::vector(view->begin(), view->end()));
    } else {
      return std::nullopt;
    }
  } else {
    return view;
  }
}

template <typename T>
static PropertyValueViewToCopy<T> propertyValueViewToCopy(const T& view) {
  if constexpr (IsMetadataNumericArray<T>::value) {
    return PropertyValueViewToCopy<T>(std::vector(view.begin(), view.end()));
  } else {
    return view;
  }
}

template <typename T>
static PropertyValueCopyToView<T> propertyValueCopyToView(const T& copy) {
  if constexpr (IsMetadataNumericArray<T>::value) {
    return copy.view();
  } else {
    return copy;
  }
}

} // namespace CesiumGltf
