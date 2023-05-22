#pragma once

#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/StructuralMetadataPropertyType.h"
#include "CesiumGltf/StructuralMetadataPropertyView.h"

#include <glm/common.hpp>

#include <optional>

namespace CesiumGltf {
namespace StructuralMetadata {

/**
 * @brief Utility to retrieve the data of
 * ExtensionExtStructuralMetadataPropertyTable.
 *
 * This should be used to get a {@link MetadataPropertyView} of a property.
 * It will validate the EXT_structural_metadata format and ensure {@link MetadataPropertyView}
 *  does not access out of bounds.
 */

class MetadataPropertyTableView {
public:
  /**
   * @brief Create an instance of MetadataPropertyTableView.
   * @param pModel The Gltf Model that stores property table data
   * @param pPropertyTable The ExtensionExtStructuralMetadataPropertyTable from
   * which the view will retrieve data.
   */
  MetadataPropertyTableView(
      const Model* pModel,
      const ExtensionExtStructuralMetadataPropertyTable* pPropertyTable);

  /**
   * @brief Find the {@link ExtensionExtStructuralMetadataClassProperty} which
   * stores the type information of a property based on the property's name.
   * @param propertyName The name of the property to retrieve type info
   * @return Pointer to the ExtensionExtStructuralMetadataClassProperty. Return
   * nullptr if no property was found.
   */
  const ExtensionExtStructuralMetadataClassProperty*
  getClassProperty(const std::string& propertyName) const;

  /**
   * @brief Gets a MetadataPropertyView to view the data of a property stored in
   * the ExtensionExtStructuralMetadataPropertyTable.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * MetadataPropertyView retrieves the correct data. T must be one of the
   * following: a scalar (uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t,
   * uint64_t, int64_t, float, double), a glm vecN composed of one of the scalar
   * types, a glm matN composed of one of the scalar types, bool,
   * std::string_view, or MetadataArrayView<T> with T as one of the
   * aforementioned types.
   *
   * @param propertyName The name of the property to retrieve data from
   * @return MetadataPropertyView of a property. The property view will be
   * invalid if no property is found.
   */
  template <typename T>
  MetadataPropertyView<T>
  getPropertyView(const std::string& propertyName) const {
    if (_pPropertyTable->count <= 0) {
      return createInvalidPropertyView<T>(
          StructuralMetadata::MetadataPropertyViewStatus::
              ErrorPropertyDoesNotExist);
    }

    const ExtensionExtStructuralMetadataClassProperty* pClassProperty =
        getClassProperty(propertyName);
    if (!pClassProperty) {
      return createInvalidPropertyView<T>(
          StructuralMetadata::MetadataPropertyViewStatus::
              ErrorPropertyDoesNotExist);
    }

    return getPropertyViewImpl<T>(propertyName, *pClassProperty);
  }

  /**
   * @brief Gets a MetadataPropertyView through a callback that accepts a
   * property name and a std::optional<MetadataPropertyView<T>> to view the data
   * of a property stored in the ExtensionExtStructuralMetadataPropertyTable.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * MetadataPropertyView retrieves the correct data. T must be one of the
   * following: a scalar (uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t,
   * uint64_t, int64_t, float, double), a glm vecN composed of one of the scalar
   * types, a glm matN composed of one of the scalar types, bool,
   * std::string_view, or MetadataArrayView<T> with T as one of the
   * aforementioned types. If the property is invalid, std::nullopt will be
   * passed to the callback. Otherwise, a valid property view will be passed to
   * the callback.
   *
   * @param propertyName The name of the property to retrieve data from
   * @tparam callback A callback function that accepts property name and
   * std::optional<MetadataPropertyView<T>>
   */
  template <typename Callback>
  void
  getPropertyView(const std::string& propertyName, Callback&& callback) const {
    const ExtensionExtStructuralMetadataClassProperty* pClassProperty =
        getClassProperty(propertyName);
    if (!pClassProperty) {
      return;
    }

    PropertyType type = convertStringToPropertyType(pClassProperty->type);
    PropertyComponentType componentType = PropertyComponentType::None;
    if (pClassProperty->componentType) {
      componentType =
          convertStringToPropertyComponentType(*pClassProperty->componentType);
    }

    if (pClassProperty->array) {
      getArrayPropertyViewImpl(
          propertyName,
          *pClassProperty,
          type,
          componentType,
          std::forward<Callback>(callback));
    } else if (isPropertyTypeVecN(type)) {
      getVecNPropertyViewImpl(
          propertyName,
          *pClassProperty,
          type,
          componentType,
          std::forward<Callback>(callback));
    } else if (isPropertyTypeMatN(type)) {
      getMatNPropertyViewImpl(
          propertyName,
          *pClassProperty,
          type,
          componentType,
          std::forward<Callback>(callback));
    } else {
      getPrimitivePropertyViewImpl(
          propertyName,
          *pClassProperty,
          type,
          componentType,
          std::forward<Callback>(callback));
    }
  }

  /**
   * @brief Iterates over each property in the
   * ExtensionExtStructuralMetadataPropertyTable with a callback that accepts a
   * property name and a std::optional<MetadataPropertyView<T>> to view the data
   * stored in the ExtensionExtStructuralMetadataPropertyTableProperty.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * MetadataPropertyView retrieves the correct data. T must be one of the
   * following: a scalar (uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t,
   * uint64_t, int64_t, float, double), a glm vecN composed of one of the scalar
   * types, a glm matN composed of one of the scalar types, bool,
   * std::string_view, or MetadataArrayView<T> with T as one of the
   * aforementioned types. If the property is invalid, std::nullopt will be
   * passed to the callback. Otherwise, a valid property view will be passed to
   * the callback.
   *
   * @param propertyName The name of the property to retrieve data from
   * @tparam callback A callback function that accepts property name and
   * std::optional<MetadataPropertyView<T>>
   */
  template <typename Callback> void forEachProperty(Callback&& callback) const {
    for (const auto& property : this->_pClass->properties) {
      getPropertyView(property.first, std::forward<Callback>(callback));
    }
  }

private:
  glm::length_t getDimensionsFromType(PropertyType type) const {
    switch (type) {
    case PropertyType::Vec2:
    case PropertyType::Mat2:
      return 2;
    case PropertyType::Vec3:
    case PropertyType::Mat3:
      return 3;
    case PropertyType::Vec4:
    case PropertyType::Mat4:
      return 4;
    default:
      return 0;
    }
  }

  template <typename Callback>
  void getScalarArrayPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<int8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<uint8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<int16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<uint16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<int32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<uint32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<int64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<uint64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<float>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<double>>(
              propertyName,
              classProperty));
      break;
    default:
      break;
    }
  }

  template <typename Callback, glm::length_t N>
  void getVecNArrayPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, int8_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, uint8_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, int16_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, uint16_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, int32_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, uint32_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, int64_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, uint64_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, float>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::vec<N, double>>>(
              propertyName,
              classProperty));
      break;
    default:
      break;
    }
  }

  template <typename Callback>
  void getVecNArrayPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    glm::length_t N = getDimensionsFromType(type);
    switch (N) {
    case 2:
      getVecNArrayPropertyViewImpl<Callback, 2>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 3:
      getVecNArrayPropertyViewImpl<Callback, 3>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 4:
      getVecNArrayPropertyViewImpl<Callback, 4>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    default:
      break;
    }
  }

  template <typename Callback, glm::length_t N>
  void getMatNArrayPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, int8_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, uint8_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, int16_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, uint16_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, int32_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, uint32_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, int64_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, uint64_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, float>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<glm::mat<N, N, double>>>(
              propertyName,
              classProperty));
      break;
    default:
      break;
    }
  }

  template <typename Callback>
  void getMatNArrayPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    const glm::length_t N = getDimensionsFromType(type);
    switch (N) {
    case 2:
      getMatNArrayPropertyViewImpl<Callback, 2>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 3:
      getMatNArrayPropertyViewImpl<Callback, 3>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 4:
      getMatNArrayPropertyViewImpl<Callback, 4>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    default:
      break;
    }
  }

  template <typename Callback>
  void getArrayPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    if (type == PropertyType::Scalar) {
      getScalarArrayPropertyViewImpl(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
    } else if (isPropertyTypeVecN(type)) {
      getVecNArrayPropertyViewImpl(
          propertyName,
          classProperty,
          type,
          componentType,
          std::forward<Callback>(callback));
    } else if (isPropertyTypeMatN(type)) {
      getMatNArrayPropertyViewImpl(
          propertyName,
          classProperty,
          type,
          componentType,
          std::forward<Callback>(callback));

    } else if (type == PropertyType::Boolean) {
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<bool>>(
              propertyName,
              classProperty));

    } else if (type == PropertyType::String) {
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<std::string_view>>(
              propertyName,
              classProperty));
    }
  }

  template <typename Callback, glm::length_t N>
  void getVecNPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {

    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, int8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, uint8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, int16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, uint16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, int32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, uint32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, int64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, uint64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, float>>(propertyName, classProperty));
      break;
    case PropertyComponentType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<glm::vec<N, double>>(
              propertyName,
              classProperty));
      break;
    default:
      break;
    }
  }

  template <typename Callback>
  void getVecNPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    const glm::length_t N = getDimensionsFromType(type);
    switch (N) {
    case 2:
      getVecNPropertyViewImpl<Callback, 2>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 3:
      getVecNPropertyViewImpl<Callback, 3>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 4:
      getVecNPropertyViewImpl<Callback, 4>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    default:
      break;
    }
  }

  template <typename Callback, glm::length_t N>
  void getMatNPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, int8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, uint8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, int16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, uint16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, int32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, uint32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, int64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, uint64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, float>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<glm::mat<N, N, double>>(
              propertyName,
              classProperty));
      break;
    default:
      break;
    }
  }

  template <typename Callback>
  void getMatNPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    glm::length_t N = getDimensionsFromType(type);
    switch (N) {
    case 2:
      getMatNPropertyViewImpl<Callback, 2>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 3:
      getMatNPropertyViewImpl<Callback, 3>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 4:
      getMatNPropertyViewImpl<Callback, 4>(
          propertyName,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    default:
      break;
    }
  }

  template <typename Callback>
  void getPrimitivePropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    if (type == PropertyType::Scalar) {
      switch (componentType) {
      case PropertyComponentType::Int8:
        callback(
            propertyName,
            getPropertyViewImpl<int8_t>(propertyName, classProperty));
        break;
      case PropertyComponentType::Uint8:
        callback(
            propertyName,
            getPropertyViewImpl<uint8_t>(propertyName, classProperty));
        break;
      case PropertyComponentType::Int16:
        callback(
            propertyName,
            getPropertyViewImpl<int16_t>(propertyName, classProperty));
        break;
      case PropertyComponentType::Uint16:
        callback(
            propertyName,
            getPropertyViewImpl<uint16_t>(propertyName, classProperty));
        break;
      case PropertyComponentType::Int32:
        callback(
            propertyName,
            getPropertyViewImpl<int32_t>(propertyName, classProperty));
        break;
      case PropertyComponentType::Uint32:
        callback(
            propertyName,
            getPropertyViewImpl<uint32_t>(propertyName, classProperty));
        break;
      case PropertyComponentType::Int64:
        callback(
            propertyName,
            getPropertyViewImpl<int64_t>(propertyName, classProperty));
        break;
      case PropertyComponentType::Uint64:
        callback(
            propertyName,
            getPropertyViewImpl<uint64_t>(propertyName, classProperty));
        break;
      case PropertyComponentType::Float32:
        callback(
            propertyName,
            getPropertyViewImpl<float>(propertyName, classProperty));
        break;
      case PropertyComponentType::Float64:
        callback(
            propertyName,
            getPropertyViewImpl<double>(propertyName, classProperty));
        break;
      default:
        break;
      }
    } else if (type == PropertyType::String) {
      callback(
          propertyName,
          getPropertyViewImpl<std::string_view>(propertyName, classProperty));
    } else if (type == PropertyType::Boolean) {
      callback(
          propertyName,
          getPropertyViewImpl<bool>(propertyName, classProperty));
    }
  }

  template <typename T>
  MetadataPropertyView<T> getPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty) const {
    auto propertyTablePropertyIter =
        _pPropertyTable->properties.find(propertyName);
    if (propertyTablePropertyIter == _pPropertyTable->properties.end()) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::ErrorPropertyDoesNotExist);
    }

    const ExtensionExtStructuralMetadataPropertyTableProperty&
        propertyTableProperty = propertyTablePropertyIter->second;

    if constexpr (IsMetadataNumeric<T>::value || IsMetadataBoolean<T>::value) {
      return getNumericOrBooleanPropertyValues<T>(
          classProperty,
          propertyTableProperty);
    }

    if constexpr (IsMetadataString<T>::value) {
      return getStringPropertyValues(classProperty, propertyTableProperty);
    }

    if constexpr (
        IsMetadataNumericArray<T>::value || IsMetadataBooleanArray<T>::value) {
      return getPrimitiveArrayPropertyValues<
          typename MetadataArrayType<T>::type>(
          classProperty,
          propertyTableProperty);
    }

    if constexpr (IsMetadataStringArray<T>::value) {
      return getStringArrayPropertyValues(classProperty, propertyTableProperty);
    }
  }

  template <typename T>
  MetadataPropertyView<T> getNumericOrBooleanPropertyValues(
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      const ExtensionExtStructuralMetadataPropertyTableProperty&
          propertyTableProperty) const {
    if (classProperty.array) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::ErrorTypeMismatch);
    }
    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::ErrorComponentTypeMismatch);
    }

    gsl::span<const std::byte> values;
    const auto status = getBufferSafe(propertyTableProperty.values, values);
    if (status != MetadataPropertyViewStatus::Valid) {
      return createInvalidPropertyView<T>(status);
    }

    if (values.size() % sizeof(T) != 0) {
      return createInvalidPropertyView<T>(
          StructuralMetadata::MetadataPropertyViewStatus::
              ErrorBufferViewSizeNotDivisibleByTypeSize);
    }

    size_t maxRequiredBytes = 0;
    if (IsMetadataBoolean<T>::value) {
      maxRequiredBytes = static_cast<size_t>(
          glm::ceil(static_cast<double>(_pPropertyTable->count) / 8.0));
    } else {
      maxRequiredBytes = _pPropertyTable->count * sizeof(T);
    }

    if (values.size() < maxRequiredBytes) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::
              ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
    }

    return MetadataPropertyView<T>(
        MetadataPropertyViewStatus::Valid,
        values,
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        PropertyComponentType::None,
        PropertyComponentType::None,
        0,
        _pPropertyTable->count,
        classProperty.normalized);
  }

  MetadataPropertyView<std::string_view> getStringPropertyValues(
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      const ExtensionExtStructuralMetadataPropertyTableProperty&
          propertyTableProperty) const;

  template <typename T>
  MetadataPropertyView<MetadataArrayView<T>> getPrimitiveArrayPropertyValues(
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      const ExtensionExtStructuralMetadataPropertyTableProperty&
          propertyTableProperty) const {
    if (!classProperty.array) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::ErrorComponentTypeMismatch);
    }

    gsl::span<const std::byte> values;
    auto status = getBufferSafe(propertyTableProperty.values, values);
    if (status != MetadataPropertyViewStatus::Valid) {
      return createInvalidPropertyView<MetadataArrayView<T>>(status);
    }

    if (values.size() % sizeof(T) != 0) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::
              ErrorBufferViewSizeNotDivisibleByTypeSize);
    }

    const int64_t fixedLengthArrayCount = classProperty.count.value_or(0);
    if (fixedLengthArrayCount > 0 && propertyTableProperty.arrayOffsets >= 0) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
    }

    if (fixedLengthArrayCount <= 0 && propertyTableProperty.arrayOffsets < 0) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist);
    }

    // Handle fixed-length arrays
    if (fixedLengthArrayCount > 0) {
      size_t maxRequiredBytes = 0;
      if constexpr (IsMetadataBoolean<T>::value) {
        maxRequiredBytes = static_cast<size_t>(glm::ceil(
            static_cast<double>(
                _pPropertyTable->count * fixedLengthArrayCount) /
            8.0));
      } else {
        maxRequiredBytes = static_cast<size_t>(
            _pPropertyTable->count * fixedLengthArrayCount * sizeof(T));
      }

      if (values.size() < maxRequiredBytes) {
        return createInvalidPropertyView<MetadataArrayView<T>>(
            MetadataPropertyViewStatus::
                ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
      }

      return MetadataPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::Valid,
          values,
          gsl::span<const std::byte>(),
          gsl::span<const std::byte>(),
          PropertyComponentType::None,
          PropertyComponentType::None,
          static_cast<size_t>(fixedLengthArrayCount),
          static_cast<size_t>(_pPropertyTable->count),
          classProperty.normalized);
    }

    // Handle variable-length arrays
    const PropertyComponentType arrayOffsetType =
        convertArrayOffsetTypeStringToPropertyComponentType(
            propertyTableProperty.arrayOffsetType);
    if (arrayOffsetType == PropertyComponentType::None) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);
    }

    constexpr bool checkBitsSize = IsMetadataBoolean<T>::value;
    gsl::span<const std::byte> arrayOffsets;
    status = getArrayOffsetsBufferSafe(
        propertyTableProperty.arrayOffsets,
        arrayOffsetType,
        values.size(),
        static_cast<size_t>(_pPropertyTable->count),
        checkBitsSize,
        arrayOffsets);
    if (status != MetadataPropertyViewStatus::Valid) {
      return createInvalidPropertyView<MetadataArrayView<T>>(status);
    }

    return MetadataPropertyView<MetadataArrayView<T>>(
        MetadataPropertyViewStatus::Valid,
        values,
        arrayOffsets,
        gsl::span<const std::byte>(),
        arrayOffsetType,
        PropertyComponentType::None,
        0,
        static_cast<size_t>(_pPropertyTable->count),
        classProperty.normalized);
  }

  MetadataPropertyView<MetadataArrayView<std::string_view>>
  getStringArrayPropertyValues(
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      const ExtensionExtStructuralMetadataPropertyTableProperty&
          propertyTableProperty) const;

  MetadataPropertyViewStatus getBufferSafe(
      int32_t bufferView,
      gsl::span<const std::byte>& buffer) const noexcept;

  MetadataPropertyViewStatus getArrayOffsetsBufferSafe(
      int32_t arrayOffsetsBufferView,
      PropertyComponentType arrayOffsetType,
      size_t valuesBufferSize,
      size_t propertyTableCount,
      bool checkBitsSize,
      gsl::span<const std::byte>& arrayOffsetsBuffer) const noexcept;

  MetadataPropertyViewStatus getStringOffsetsBufferSafe(
      int32_t stringOffsetsBufferView,
      PropertyComponentType stringOffsetType,
      size_t valuesBufferSize,
      size_t propertyTableCount,
      gsl::span<const std::byte>& stringOffsetsBuffer) const noexcept;

  template <typename T>
  static MetadataPropertyView<T>
  createInvalidPropertyView(MetadataPropertyViewStatus invalidStatus) noexcept {
    return MetadataPropertyView<T>(
        invalidStatus,
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        PropertyComponentType::None,
        PropertyComponentType::None,
        0,
        0,
        false);
  }

  const Model* _pModel;
  const ExtensionExtStructuralMetadataPropertyTable* _pPropertyTable;
  const ExtensionExtStructuralMetadataClass* _pClass;
};

} // namespace StructuralMetadata
} // namespace CesiumGltf
