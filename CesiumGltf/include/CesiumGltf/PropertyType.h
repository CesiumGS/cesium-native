#pragma once

#include <glm/common.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace CesiumGltf {
/**
 * @brief The possible types of a property in a PropertyTableView.
 */
enum class PropertyType {
  /**
   * @brief An invalid property type.
   */
  Invalid,
  /**
   * @brief A scalar property, i.e. an integer or floating point value.
   */
  Scalar,
  /**
   * @brief A vector with two components.
   */
  Vec2,
  /**
   * @brief A vector with three components.
   */
  Vec3,
  /**
   * @brief A vector with four components.
   */
  Vec4,
  /**
   * @brief A 2x2 matrix.
   */
  Mat2,
  /**
   * @brief A 3x3 matrix.
   */
  Mat3,
  /**
   * @brief A 4x4 matrix.
   */
  Mat4,
  /**
   * @brief A string.
   */
  String,
  /**
   * @brief A boolean.
   */
  Boolean,
  /**
   * @brief An enum.
   */
  Enum
};

/**
 * @brief The possible types of a property component.
 */
enum class PropertyComponentType {
  /**
   * @brief No type.
   */
  None,
  /**
   * @brief A property component equivalent to an `int8_t`.
   */
  Int8,
  /**
   * @brief A property component equivalent to a `uint8_t`.
   */
  Uint8,
  /**
   * @brief A property component equivalent to an `int16_t`.
   */
  Int16,
  /**
   * @brief A property component equivalent to a `uint16_t`.
   */
  Uint16,
  /**
   * @brief A property component equivalent to an `int32_t`.
   */
  Int32,
  /**
   * @brief A property component equivalent to a `uint32_t`.
   */
  Uint32,
  /**
   * @brief A property component equivalent to an `int64_t`.
   */
  Int64,
  /**
   * @brief A property component equivalent to a `uint32_t`.
   */
  Uint64,
  /**
   * @brief A property component equivalent to a `float`.
   */
  Float32,
  /**
   * @brief A property component equivalent to a `double`.
   */
  Float64,
};

/**
 * @brief Converts a \ref PropertyType value to a string.
 *
 * For example, \ref PropertyType::Mat3 will become `"MAT3"`.
 *
 * @param type The type to convert to a string.
 * @returns The type as a string, or `"INVALID"` if no conversion is possible.
 */
std::string convertPropertyTypeToString(PropertyType type);

/**
 * @brief Converts a string into a \ref PropertyType.
 *
 * For example, `"MAT3"` will become a \ref PropertyType::Mat3.
 *
 * @param str The string to convert to a \ref PropertyType.
 * @returns The string as a \ref PropertyType, or \ref PropertyType::Invalid if
 * no conversion is possible.
 */
PropertyType convertStringToPropertyType(const std::string& str);

/**
 * @brief Converts a \ref PropertyComponentType value to a string.
 *
 * For example, \ref PropertyComponentType::Uint8 will become `"UINT8"`.
 *
 * @param componentType The type to convert to a string.
 * @returns The type as a string, or `"NONE"` if no conversion is possible.
 */
std::string
convertPropertyComponentTypeToString(PropertyComponentType componentType);

/**
 * @brief Converts a string into a \ref PropertyComponentType.
 *
 * For example, `"UINT8"` will become a \ref PropertyComponentType::Uint8.
 *
 * @param str The string to convert to a \ref PropertyComponentType.
 * @returns The string as a \ref PropertyComponentType, or \ref
 * PropertyComponentType::None if no conversion is possible.
 */
PropertyComponentType
convertStringToPropertyComponentType(const std::string& str);

/**
 * @brief Converts a string listed in \ref
 * PropertyTableProperty::ArrayOffsetType to its corresponding \ref
 * PropertyComponentType.
 *
 * @param str The string to convert to a \ref PropertyComponentType.
 * @returns The string as a \ref PropertyComponentType, or \ref
 * PropertyComponentType::None if no conversion is possible.
 */
PropertyComponentType
convertArrayOffsetTypeStringToPropertyComponentType(const std::string& str);

/**
 * @brief Converts a string listed in \ref
 * PropertyTableProperty::StringOffsetType to its corresponding \ref
 * PropertyComponentType.
 *
 * @param str The string to convert to a \ref PropertyComponentType.
 * @returns The string as a \ref PropertyComponentType, or \ref
 * PropertyComponentType::None if no conversion is possible.
 */
PropertyComponentType
convertStringOffsetTypeStringToPropertyComponentType(const std::string& str);

/**
 * @brief Converts a integer type ID listed in \ref
 * AccessorSpec::ComponentType to its corresponding \ref
 * PropertyComponentType.
 *
 * @param componentType The integer ID to convert to a \ref
 * PropertyComponentType.
 * @returns The ID as a \ref PropertyComponentType, or \ref
 * PropertyComponentType::None if no conversion is possible.
 */
PropertyComponentType
convertAccessorComponentTypeToPropertyComponentType(int componentType);

/**
 * @brief Converts a \ref PropertyComponentType to an integer type ID listed in
 * \ref AccessorSpec::ComponentType.
 *
 * @param componentType The \ref PropertyComponentType to convert to an integer
 * type ID.
 * @returns The integer type ID listed in \ref AccessorSpec::ComponentType, or
 * -1 if no conversion is possible.
 */
int32_t convertPropertyComponentTypeToAccessorComponentType(
    PropertyComponentType componentType);

/**
 * @brief Checks if the given \ref PropertyType represents a vector with any
 * number of components.
 *
 * @param type The \ref PropertyType to check.
 * @returns True if \ref PropertyType is \ref PropertyType::Vec2, \ref
 * PropertyType::Vec3, or \ref PropertyType::Vec4, or false otherwise.
 */
bool isPropertyTypeVecN(PropertyType type);

/**
 * @brief Checks if the given \ref PropertyType represents a matrix with any
 * number of components.
 *
 * @param type The \ref PropertyType to check.
 * @returns True if \ref PropertyType is \ref PropertyType::Mat2, \ref
 * PropertyType::Mat3, or \ref PropertyType::Mat4, or false otherwise.
 */
bool isPropertyTypeMatN(PropertyType type);

/**
 * @brief Checks if the given \ref PropertyComponentType represents an integer
 * value.
 *
 * @param componentType The \ref PropertyComponentType to check.
 * @returns True if the \ref PropertyComponentType is \b not \ref
 * PropertyComponentType::Float32 or \ref PropertyComponentType::Float64, false
 * otherwise.
 */
bool isPropertyComponentTypeInteger(PropertyComponentType componentType);

/**
 * @brief Obtains the number of dimensions in the given \ref PropertyType.
 *
 * @param type The type of the property to get the dimensions of.
 * @returns The number of dimensions in a value of the given \ref PropertyType.
 * For example, a value of \ref PropertyType::Scalar has one dimension. A value
 * of \ref PropertyType::Mat4 and a value of \ref PropertyType::Vec4 both have
 * four dimensions.
 */
glm::length_t getDimensionsFromPropertyType(PropertyType type);

/**
 * @brief Obtains the number of components in the given \ref PropertyType.
 *
 * @param type The type of the property to get the component count of.
 * @returns The number of components in a value of the given \ref PropertyType.
 * For example, a value of \ref PropertyType::Scalar has one component. A value
 * of \ref PropertyType::Vec4 has four components. A value of \ref
 * PropertyType::Mat4 has 16 components.
 */
glm::length_t getComponentCountFromPropertyType(PropertyType type);

/**
 * @brief Obtains the size in bytes of a value of this \ref
 * PropertyComponentType.
 *
 * @param componentType The component type to get the size of.
 * @returns The size in bytes that a value matching this \ref
 * PropertyComponentType. For example, a value of \ref
 * PropertyComponentType::Uint32 would take up four bytes.
 */
size_t getSizeOfComponentType(PropertyComponentType componentType);

} // namespace CesiumGltf
