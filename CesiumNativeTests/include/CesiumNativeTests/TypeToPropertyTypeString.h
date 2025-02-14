#pragma once

#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/PropertyType.h>

/**
 * @brief Converts a C++ type to a field in \ref ClassProperty::Type.
 *
 * This is useful when creating a generic \ref ClassProperty in a test with type
 * information from a template parameter.
 */
template <typename T> struct TypeToPropertyTypeString;

template <> struct TypeToPropertyTypeString<int8_t> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<uint8_t> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<int16_t> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<uint16_t> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<int32_t> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<uint32_t> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<int64_t> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<uint64_t> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<float> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<double> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::SCALAR;
};
template <> struct TypeToPropertyTypeString<std::string_view> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::STRING;
};
template <> struct TypeToPropertyTypeString<bool> {
  inline static const std::string value =
      CesiumGltf::ClassProperty::Type::BOOLEAN;
};
template <typename T, glm::qualifier P>
struct TypeToPropertyTypeString<glm::vec<2, T, P>> {
  inline static const std::string value = CesiumGltf::ClassProperty::Type::VEC2;
};
template <typename T, glm::qualifier P>
struct TypeToPropertyTypeString<glm::vec<3, T, P>> {
  inline static const std::string value = CesiumGltf::ClassProperty::Type::VEC3;
};
template <typename T, glm::qualifier P>
struct TypeToPropertyTypeString<glm::vec<4, T, P>> {
  inline static const std::string value = CesiumGltf::ClassProperty::Type::VEC4;
};
template <typename T, glm::qualifier P>
struct TypeToPropertyTypeString<glm::mat<2, 2, T, P>> {
  inline static const std::string value = CesiumGltf::ClassProperty::Type::MAT2;
};
template <typename T, glm::qualifier P>
struct TypeToPropertyTypeString<glm::mat<3, 3, T, P>> {
  inline static const std::string value = CesiumGltf::ClassProperty::Type::MAT3;
};
template <typename T, glm::qualifier P>
struct TypeToPropertyTypeString<glm::mat<4, 4, T, P>> {
  inline static const std::string value = CesiumGltf::ClassProperty::Type::MAT4;
};