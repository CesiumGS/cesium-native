#pragma once

#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyType.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <type_traits>

namespace CesiumGltf {
/**
 * @brief Check if a C++ type can be represented as a scalar property type
 */
template <typename... T> struct IsMetadataScalar;
/** @copydoc IsMetadataScalar */
template <typename T> struct IsMetadataScalar<T> : std::false_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<uint8_t> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<int8_t> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<uint16_t> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<int16_t> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<uint32_t> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<int32_t> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<uint64_t> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<int64_t> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<float> : std::true_type {};
/** @copydoc IsMetadataScalar */
template <> struct IsMetadataScalar<double> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an integer property type
 */
template <typename... T> struct IsMetadataInteger;
/** @copydoc IsMetadataInteger */
template <typename T> struct IsMetadataInteger<T> : std::false_type {};
/** @copydoc IsMetadataInteger */
template <> struct IsMetadataInteger<uint8_t> : std::true_type {};
/** @copydoc IsMetadataInteger */
template <> struct IsMetadataInteger<int8_t> : std::true_type {};
/** @copydoc IsMetadataInteger */
template <> struct IsMetadataInteger<uint16_t> : std::true_type {};
/** @copydoc IsMetadataInteger */
template <> struct IsMetadataInteger<int16_t> : std::true_type {};
/** @copydoc IsMetadataInteger */
template <> struct IsMetadataInteger<uint32_t> : std::true_type {};
/** @copydoc IsMetadataInteger */
template <> struct IsMetadataInteger<int32_t> : std::true_type {};
/** @copydoc IsMetadataInteger */
template <> struct IsMetadataInteger<uint64_t> : std::true_type {};
/** @copydoc IsMetadataInteger */
template <> struct IsMetadataInteger<int64_t> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a floating-point property
 * type.
 */
template <typename... T> struct IsMetadataFloating;
/** @copydoc IsMetadataFloating */
template <typename T> struct IsMetadataFloating<T> : std::false_type {};
/** @copydoc IsMetadataFloating */
template <> struct IsMetadataFloating<float> : std::true_type {};
/** @copydoc IsMetadataFloating */
template <> struct IsMetadataFloating<double> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a vecN type.
 */
template <typename... T> struct IsMetadataVecN;
/** @copydoc IsMetadataVecN */
template <typename T> struct IsMetadataVecN<T> : std::false_type {};
/** @copydoc IsMetadataVecN */
template <glm::length_t n, typename T, glm::qualifier P>
struct IsMetadataVecN<glm::vec<n, T, P>> : IsMetadataScalar<T> {};

/**
 * @brief Check if a C++ type can be represented as a matN type.
 */
template <typename... T> struct IsMetadataMatN;
/** @copydoc IsMetadataMatN */
template <typename T> struct IsMetadataMatN<T> : std::false_type {};
/** @copydoc IsMetadataMatN */
template <glm::length_t n, typename T, glm::qualifier P>
struct IsMetadataMatN<glm::mat<n, n, T, P>> : IsMetadataScalar<T> {};

/**
 * @brief Check if a C++ type can be represented as a numeric property, i.e.
 * a scalar / vecN / matN type.
 */
template <typename... T> struct IsMetadataNumeric;
/**
 * @copydoc IsMetadataNumeric
 */
template <typename T> struct IsMetadataNumeric<T> {
  /**
   * @brief Whether the given metadata type is a scalar, a vector, or a matrix.
   */
  static constexpr bool value = IsMetadataScalar<T>::value ||
                                IsMetadataVecN<T>::value ||
                                IsMetadataMatN<T>::value;
};

/**
 * @brief Check if a C++ type can be represented as a boolean property type
 */
template <typename... T> struct IsMetadataBoolean;
/** @copydoc IsMetadataBoolean */
template <typename T> struct IsMetadataBoolean<T> : std::false_type {};
/** @copydoc IsMetadataBoolean */
template <> struct IsMetadataBoolean<bool> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a string property type
 */
template <typename... T> struct IsMetadataString;
/** @copydoc IsMetadataString */
template <typename T> struct IsMetadataString<T> : std::false_type {};
/** @copydoc IsMetadataString */
template <> struct IsMetadataString<std::string_view> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array.
 */
template <typename... T> struct IsMetadataArray;
/** @copydoc IsMetadataArray */
template <typename T> struct IsMetadataArray<T> : std::false_type {};
/** @copydoc IsMetadataArray */
template <typename T>
struct IsMetadataArray<PropertyArrayView<T>> : std::true_type {};
/** @copydoc IsMetadataArray */
template <typename T>
struct IsMetadataArray<PropertyArrayCopy<T>> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array of numeric elements
 * property type
 */
template <typename... T> struct IsMetadataNumericArray;
/** @copydoc IsMetadataNumericArray */
template <typename T> struct IsMetadataNumericArray<T> : std::false_type {};
/** @copydoc IsMetadataNumericArray */
template <typename T> struct IsMetadataNumericArray<PropertyArrayView<T>> {
  /** @brief Whether the component of this \ref PropertyArrayView is numeric. */
  static constexpr bool value = IsMetadataNumeric<T>::value;
};
/** @copydoc IsMetadataNumericArray */
template <typename T> struct IsMetadataNumericArray<PropertyArrayCopy<T>> {
  /** @brief Whether the component of this \ref PropertyArrayCopy is numeric. */
  static constexpr bool value = IsMetadataNumeric<T>::value;
};

/**
 * @brief Check if a C++ type can be represented as an array of booleans
 * property type
 */
template <typename... T> struct IsMetadataBooleanArray;
/** @copydoc IsMetadataBooleanArray */
template <typename T> struct IsMetadataBooleanArray<T> : std::false_type {};
/** @copydoc IsMetadataBooleanArray */
template <>
struct IsMetadataBooleanArray<PropertyArrayView<bool>> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array of strings property
 * type
 */
template <typename... T> struct IsMetadataStringArray;
/** @copydoc IsMetadataStringArray */
template <typename T> struct IsMetadataStringArray<T> : std::false_type {};
/** @copydoc IsMetadataStringArray */
template <>
struct IsMetadataStringArray<PropertyArrayView<std::string_view>>
    : std::true_type {};

/**
 * @brief Retrieve the component type of a metadata array
 */
template <typename T> struct MetadataArrayType {
  /** @brief The component type of this metadata array. */
  using type = void;
};
/** @copydoc MetadataArrayType */
template <typename T>
struct MetadataArrayType<CesiumGltf::PropertyArrayView<T>> {
  /** @brief The component type of this metadata array. */
  using type = T;
};
/** @copydoc MetadataArrayType */
template <typename T>
struct MetadataArrayType<CesiumGltf::PropertyArrayCopy<T>> {
  /** @brief The component type of this metadata array. */
  using type = T;
};

/**
 * @brief Convert a C++ type to PropertyType and PropertyComponentType
 */
template <typename T> struct TypeToPropertyType;

#pragma region Scalar Property Types

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<uint8_t> {
  /** @brief The \ref PropertyComponentType corresponding to a `uint8_t`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  /** @brief The \ref PropertyType corresponding to a `uint8_t`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<int8_t> {
  /** @brief The \ref PropertyComponentType corresponding to an `int8_t`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  /** @brief The \ref PropertyType corresponding to an `int8_t`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<uint16_t> {
  /** @brief The \ref PropertyComponentType corresponding to a `uint16_t`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  /** @brief The \ref PropertyType corresponding to a `uint16_t`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<int16_t> {
  /** @brief The \ref PropertyComponentType corresponding to an `int16_t`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  /** @brief The \ref PropertyType corresponding to an `int16_t`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<uint32_t> {
  /** @brief The \ref PropertyComponentType corresponding to a `uint32_t`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  /** @brief The \ref PropertyType corresponding to a `uint32_t`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<int32_t> {
  /** @brief The \ref PropertyComponentType corresponding to an `int32_t`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  /** @brief The \ref PropertyType corresponding to an `int32_t`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<uint64_t> {
  /** @brief The \ref PropertyComponentType corresponding to a `uint64_t`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  /** @brief The \ref PropertyType corresponding to a `uint64_t`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<int64_t> {
  /** @brief The \ref PropertyComponentType corresponding to an `int64_t`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  /** @brief The \ref PropertyType corresponding to an `int64_t`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<float> {
  /** @brief The \ref PropertyComponentType corresponding to a `float`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  /** @brief The \ref PropertyType corresponding to a `float`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<double> {
  /** @brief The \ref PropertyComponentType corresponding to a `double`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  /** @brief The \ref PropertyType corresponding to a `float`. */
  static constexpr PropertyType value = PropertyType::Scalar;
};
#pragma endregion

#pragma region Vector Property Types

/** @copydoc TypeToPropertyType */
template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<2, T, P>> {
  /** @brief The \ref PropertyComponentType corresponding to a `glm::vec<2,
   * ...>`. */
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  /** @brief The \ref PropertyType corresponding to a `glm::vec<2, ...>`. */
  static constexpr PropertyType value = PropertyType::Vec2;
};

/** @copydoc TypeToPropertyType */
template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<3, T, P>> {
  /** @brief The \ref PropertyComponentType corresponding to a `glm::vec<3,
   * ...>`. */
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  /** @brief The \ref PropertyType corresponding to a `glm::vec<3, ...>`. */
  static constexpr PropertyType value = PropertyType::Vec3;
};

/** @copydoc TypeToPropertyType */
template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<4, T, P>> {
  /** @brief The \ref PropertyComponentType corresponding to a `glm::vec<4,
   * ...>`. */
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  /** @brief The \ref PropertyType corresponding to a `glm::vec<4, ...>`. */
  static constexpr PropertyType value = PropertyType::Vec4;
};

#pragma endregion

#pragma region Matrix Property Types

/** @copydoc TypeToPropertyType */
template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<2, 2, T, P>> {
  /** @brief The \ref PropertyComponentType corresponding to a `glm::mat<2, 2,
   * ...>`. */
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  /** @brief The \ref PropertyType corresponding to a `glm::mat<2, 2, ...>`. */
  static constexpr PropertyType value = PropertyType::Mat2;
};

/** @copydoc TypeToPropertyType */
template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<3, 3, T, P>> {
  /** @brief The \ref PropertyComponentType corresponding to a `glm::mat<3, 3,
   * ...>`. */
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  /** @brief The \ref PropertyType corresponding to a `glm::mat<3, 3, ...>`. */
  static constexpr PropertyType value = PropertyType::Mat3;
};

/** @copydoc TypeToPropertyType */
template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<4, 4, T, P>> {
  /** @brief The \ref PropertyComponentType corresponding to a `glm::mat<4, 4,
   * ...>`. */
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  /** @brief The \ref PropertyType corresponding to a `glm::mat<4, 4, ...>`. */
  static constexpr PropertyType value = PropertyType::Mat4;
};

#pragma endregion

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<bool> {
  /** @brief The \ref PropertyComponentType corresponding to a `bool`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::None;
  /** @brief The \ref PropertyType corresponding to a `bool`. */
  static constexpr PropertyType value = PropertyType::Boolean;
};

/** @copydoc TypeToPropertyType */
template <> struct TypeToPropertyType<std::string_view> {
  /** @brief The \ref PropertyComponentType corresponding to a
   * `std::string_view`. */
  static constexpr PropertyComponentType component =
      PropertyComponentType::None;
  /** @brief The \ref PropertyType corresponding to a `std::string_view`. */
  static constexpr PropertyType value = PropertyType::String;
};

/**
 * @brief Check if a C++ type can be normalized.
 */
template <typename... T> struct CanBeNormalized;
/** @copydoc CanBeNormalized */
template <typename T> struct CanBeNormalized<T> : std::false_type {};
/** @copydoc CanBeNormalized */
template <> struct CanBeNormalized<uint8_t> : std::true_type {};
/** @copydoc CanBeNormalized */
template <> struct CanBeNormalized<int8_t> : std::true_type {};
/** @copydoc CanBeNormalized */
template <> struct CanBeNormalized<uint16_t> : std::true_type {};
/** @copydoc CanBeNormalized */
template <> struct CanBeNormalized<int16_t> : std::true_type {};
/** @copydoc CanBeNormalized */
template <> struct CanBeNormalized<uint32_t> : std::true_type {};
/** @copydoc CanBeNormalized */
template <> struct CanBeNormalized<int32_t> : std::true_type {};
/** @copydoc CanBeNormalized */
template <> struct CanBeNormalized<uint64_t> : std::true_type {};
/** @copydoc CanBeNormalized */
template <> struct CanBeNormalized<int64_t> : std::true_type {};

/** @copydoc CanBeNormalized */
template <glm::length_t n, typename T, glm::qualifier P>
struct CanBeNormalized<glm::vec<n, T, P>> : CanBeNormalized<T> {};

/** @copydoc CanBeNormalized */
template <glm::length_t n, typename T, glm::qualifier P>
struct CanBeNormalized<glm::mat<n, n, T, P>> : CanBeNormalized<T> {};

/** @copydoc CanBeNormalized */
template <typename T>
struct CanBeNormalized<PropertyArrayView<T>> : CanBeNormalized<T> {};
/**
 * @brief Convert an integer numeric type to the corresponding representation as
 * a double type. Doubles are preferred over floats to maintain more precision.
 */
template <typename T> struct TypeToNormalizedType;

/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<int8_t> {
  /** @brief The representation of an `int8_t` as a double type. */
  using type = double;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<uint8_t> {
  /** @brief The representation of a `uint8_t` as a double type. */
  using type = double;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<int16_t> {
  /** @brief The representation of an `int16_t` as a double type. */
  using type = double;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<uint16_t> {
  /** @brief The representation of a `uint16_t` as a double type. */
  using type = double;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<int32_t> {
  /** @brief The representation of an `int32_t` as a double type. */
  using type = double;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<uint32_t> {
  /** @brief The representation of a `uint32_t` as a double type. */
  using type = double;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<int64_t> {
  /** @brief The representation of an `int64_t` as a double type. */
  using type = double;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<uint64_t> {
  /** @brief The representation of a `uint64_t` as a double type. */
  using type = double;
};

/** @copydoc TypeToNormalizedType */
template <glm::length_t N, typename T, glm::qualifier Q>
struct TypeToNormalizedType<glm::vec<N, T, Q>> {
  /** @brief The representation of a `glm::vec<N, T, Q>` as a double type. */
  using type = glm::vec<N, double, Q>;
};

/** @copydoc TypeToNormalizedType */
template <glm::length_t N, typename T, glm::qualifier Q>
struct TypeToNormalizedType<glm::mat<N, N, T, Q>> {
  /** @brief The representation of a `glm::mat<N, N, T, Q>` as a double type. */
  using type = glm::mat<N, N, double, Q>;
};

/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<PropertyArrayView<int8_t>> {
  /** @brief The representation of an array of `int8_t` types as an array of its
   * double type equivalents. */
  using type = PropertyArrayView<double>;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<PropertyArrayView<uint8_t>> {
  /** @brief The representation of an array of `uint8_t` types as an array of
   * its double type equivalents. */
  using type = PropertyArrayView<double>;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<PropertyArrayView<int16_t>> {
  /** @brief The representation of an array of `int16_t` types as an array of
   * its double type equivalents. */
  using type = PropertyArrayView<double>;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<PropertyArrayView<uint16_t>> {
  /** @brief The representation of an array of `uint16_t` types as an array of
   * its double type equivalents. */
  using type = PropertyArrayView<double>;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<PropertyArrayView<int32_t>> {
  /** @brief The representation of an array of `int32_t` types as an array of
   * its double type equivalents. */
  using type = PropertyArrayView<double>;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<PropertyArrayView<uint32_t>> {
  /** @brief The representation of an array of `uint32_t` types as an array of
   * its double type equivalents. */
  using type = PropertyArrayView<double>;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<PropertyArrayView<int64_t>> {
  /** @brief The representation of an array of `int64_t` types as an array of
   * its double type equivalents. */
  using type = PropertyArrayView<double>;
};
/** @copydoc TypeToNormalizedType */
template <> struct TypeToNormalizedType<PropertyArrayView<uint64_t>> {
  /** @brief The representation of an array of `uint64_t` types as an array of
   * its double type equivalents. */
  using type = PropertyArrayView<double>;
};

/** @copydoc TypeToNormalizedType */
template <glm::length_t N, typename T, glm::qualifier Q>
struct TypeToNormalizedType<PropertyArrayView<glm::vec<N, T, Q>>> {
  /** @brief The representation of an array of `glm::vec<N, T, Q>` types as an
   * array of its double type equivalents. */
  using type = PropertyArrayView<glm::vec<N, double, Q>>;
};

/** @copydoc TypeToNormalizedType */
template <glm::length_t N, typename T, glm::qualifier Q>
struct TypeToNormalizedType<PropertyArrayView<glm::mat<N, N, T, Q>>> {
  /** @brief The representation of an array of `glm::mat<N, N, T, Q>` types as
   * an array of its double type equivalents. */
  using type = PropertyArrayView<glm::mat<N, N, double, Q>>;
};

/**
 * @brief Transforms a property value type from a view to an equivalent type
 * that owns the data it is viewing. For most property types this is an identity
 * transformation, because most property types are held by value. However, it
 * transforms numeric `PropertyArrayView<T>` to `PropertyArrayCopy<T>` because a
 * `PropertyArrayView<T>` only has a pointer to the value it is viewing.
 *
 * See `propertyValueViewToCopy`.
 *
 * @remarks This is the inverse of \ref PropertyValueCopyToView
 * @tparam T The type of the property value view.
 */
template <typename T>
using PropertyValueViewToCopy = std::conditional_t<
    IsMetadataNumericArray<T>::value,
    PropertyArrayCopy<typename MetadataArrayType<T>::type>,
    T>;

/**
 * @brief Transforms a property value type from a copy that owns the data it is
 * viewing to a view into that data. For most property types this is an identity
 * transformation, because most property types are held by value. However, it
 * transforms numeric `PropertyArrayCopy<T>` to `PropertyArrayView<T>`.
 *
 * See `propertyValueCopyToView`.
 *
 * @remarks This is the inverse of \ref PropertyValueViewToCopy
 * @tparam T The type of the property value copy.
 */
template <typename T>
using PropertyValueCopyToView = std::conditional_t<
    IsMetadataNumericArray<T>::value,
    PropertyArrayView<typename MetadataArrayType<T>::type>,
    T>;

/**
 * @brief Creates an optional instance of a type that can be used to own a
 * property value from an optional instance that is only a view on that value.
 * See {@link PropertyValueViewToCopy}.
 *
 * @tparam T The type of the view to copy.
 * @param view An optional instance of a view on the value that will be copied.
 * @return std::optional<PropertyValueViewToCopy<T>>
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

/**
 * @brief Creates an instance of a type that will own a property value from a
 * view on that value. See \ref PropertyValueViewToOwner.
 *
 * @tparam T The type of the view to copy.
 * @param view A view on the value that will be copied.
 * @return PropertyValueViewToCopy<T>
 */
template <typename T>
static PropertyValueViewToCopy<T> propertyValueViewToCopy(const T& view) {
  if constexpr (IsMetadataNumericArray<T>::value) {
    return PropertyValueViewToCopy<T>(std::vector(view.begin(), view.end()));
  } else {
    return view;
  }
}

/**
 * @brief Creates a view on an owned copy of a property value. See \ref
 * PropertyValueCopyToView.
 *
 * @tparam T The type of the value to create a view from.
 * @param copy The value to create a view from.
 * @return PropertyValueCopyToView<T>
 */
template <typename T>
static PropertyValueCopyToView<T> propertyValueCopyToView(const T& copy) {
  if constexpr (IsMetadataNumericArray<T>::value) {
    return copy.view();
  } else {
    return copy;
  }
}

} // namespace CesiumGltf
