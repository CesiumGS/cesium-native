#pragma once

#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CesiumGltf/PropertyType.h"
#include "glm/common.hpp"
#include <optional>

namespace CesiumGltf {
/**
 * @brief Utility to retrieve the data of FeatureTable
 *
 * This should be used to get {@link MetadataPropertyView} of a property since
 * it will validate the EXT_Feature_Metadata format to make sure {@link MetadataPropertyView}
 * not access out of bound
 */
class MetadataFeatureTableView {
public:
  /**
   * @brief Create an instance of MetadataFeatureTableView
   * @param model The Gltf Model that stores featureTable data
   * @param featureTable The FeatureTable that will be used to retrieve the data
   * from
   */
  MetadataFeatureTableView(
      const Model* model,
      const FeatureTable* featureTable);

  /**
   * @brief Find the {@link ClassProperty} which stores the type information of a property based on the property name
   * @param propertyName The name of the property to retrieve type info
   * @return ClassProperty of a property. Return nullptr if no property is found
   */
  const ClassProperty* getClassProperty(const std::string& propertyName) const;

  /**
   * @brief Get MetadataPropertyView to view the data stored in the
   * FeatureTableProperty.
   *
   * This method will validate the EXT_Feature_Metadata format to ensure
   * MetadataPropertyView retrieve the correct data. T must be uin8_t, int8_t,
   * uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, bool,
   * std::string_view, and MetadataArrayView<T> with T must be one of the types
   * mentioned above
   *
   * @param propertyName The name of the property to retrieve data from
   * @return ClassProperty of a property. Return nullptr if no property is found
   */
  template <typename T>
  std::optional<MetadataPropertyView<T>>
  getPropertyView(const std::string& propertyName) const {
    if (_featureTable->count < 0) {
      return std::nullopt;
    }

    const ClassProperty* classProperty = getClassProperty(propertyName);
    if (!classProperty) {
      return std::nullopt;
    }

    return getPropertyViewImpl<T>(propertyName, classProperty);
  }

  template <typename Callback>
  void
  getPropertyView(const std::string& propertyName, Callback&& callback) const {
    const ClassProperty* classProperty = getClassProperty(propertyName);
    if (!classProperty) {
      return;
    }

    PropertyType type = convertStringToPropertyType(classProperty->type);
    PropertyType componentType = PropertyType::None;
    if (classProperty->componentType.isString()) {
      componentType =
          convertStringToPropertyType(classProperty->componentType.getString());
    }

    if (type != PropertyType::Array) {
      getScalarPropertyViewImpl(
          propertyName,
          classProperty,
          type,
          std::forward<Callback>(callback));
    } else {
      getArrayPropertyViewImpl(
          propertyName,
          classProperty,
          type,
          std::forward<Callback>(callback));
    }
  }

  template <typename Callback> void forEachProperty(Callback&& callback) const {
    for (const auto& property : this->_class->properties) {
      getPropertyView(property.first, std::forward<Callback>(callback));
    }
  }

private:
  template <typename Callback>
  void getArrayPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty* classProperty,
      PropertyType type,
      Callback&& callback) const {
    switch (type) {
    case PropertyType::Int8:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<int8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<uint8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<int16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<uint16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<int32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<uint32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<int64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<uint64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<float>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<double>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::Boolean:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<bool>>(
              propertyName,
              classProperty));
      break;
    case PropertyType::String:
      callback(
          propertyName,
          getPropertyViewImpl<MetadataArrayView<std::string_view>>(
              propertyName,
              classProperty));
      break;
    default:
      break;
    }
  }

  template <typename Callback>
  void getScalarPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty* classProperty,
      PropertyType type,
      Callback&& callback) const {
    switch (type) {
    case PropertyType::Int8:
      callback(
          propertyName,
          getPropertyViewImpl<int8_t>(propertyName, classProperty));
      break;
    case PropertyType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<uint8_t>(propertyName, classProperty));
      break;
    case PropertyType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<int16_t>(propertyName, classProperty));
      break;
    case PropertyType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<uint16_t>(propertyName, classProperty));
      break;
    case PropertyType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<int32_t>(propertyName, classProperty));
      break;
    case PropertyType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<uint32_t>(propertyName, classProperty));
      break;
    case PropertyType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<int64_t>(propertyName, classProperty));
      break;
    case PropertyType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<uint64_t>(propertyName, classProperty));
      break;
    case PropertyType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<float>(propertyName, classProperty));
      break;
    case PropertyType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<double>(propertyName, classProperty));
      break;
    case PropertyType::Boolean:
      callback(
          propertyName,
          getPropertyViewImpl<bool>(propertyName, classProperty));
      break;
    case PropertyType::String:
      callback(
          propertyName,
          getPropertyViewImpl<std::string_view>(propertyName, classProperty));
      break;
    default:
      break;
    }
  }

  template <typename T>
  std::optional<MetadataPropertyView<T>> getPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty* classProperty) const {
    auto featureTablePropertyIter =
        _featureTable->properties.find(propertyName);
    if (featureTablePropertyIter == _featureTable->properties.end()) {
      return std::nullopt;
    }

    const FeatureTableProperty& featureTableProperty =
        featureTablePropertyIter->second;

    if constexpr (IsMetadataNumeric<T>::value || IsMetadataBoolean<T>::value) {
      return getPrimitivePropertyValues<T>(classProperty, featureTableProperty);
    }

    if constexpr (IsMetadataString<T>::value) {
      return getStringPropertyValues(classProperty, featureTableProperty);
    }

    if constexpr (
        IsMetadataNumericArray<T>::value || IsMetadataBooleanArray<T>::value) {
      return getPrimitiveArrayPropertyValues<
          typename MetadataArrayType<T>::type>(
          classProperty,
          featureTableProperty);
    }

    if constexpr (IsMetadataStringArray<T>::value) {
      return getStringArrayPropertyValues(classProperty, featureTableProperty);
    }
  }

  template <typename T>
  std::optional<MetadataPropertyView<T>> getPrimitivePropertyValues(
      const ClassProperty* classProperty,
      const FeatureTableProperty& featureTableProperty) const {
    PropertyType type = convertStringToPropertyType(classProperty->type);
    if (TypeToPropertyType<T>::value != static_cast<uint32_t>(type)) {
      return std::nullopt;
    }

    gsl::span<const std::byte> valueBuffer =
        getBufferSafe(featureTableProperty.bufferView);
    if (valueBuffer.empty()) {
      return std::nullopt;
    }

    if (valueBuffer.size() % sizeof(T) != 0) {
      return std::nullopt;
    }

    size_t maxRequiredBytes = 0;
    if (IsMetadataBoolean<T>::value) {
      maxRequiredBytes = static_cast<size_t>(
          glm::ceil(static_cast<double>(_featureTable->count) / 8.0));
    } else {
      maxRequiredBytes = _featureTable->count * sizeof(T);
    }

    if (valueBuffer.size() < maxRequiredBytes) {
      return std::nullopt;
    }

    return MetadataPropertyView<T>(
        valueBuffer,
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        PropertyType::None,
        0,
        _featureTable->count);
  }

  std::optional<MetadataPropertyView<std::string_view>> getStringPropertyValues(
      const ClassProperty* classProperty,
      const FeatureTableProperty& featureTableProperty) const;

  template <typename T>
  std::optional<MetadataPropertyView<MetadataArrayView<T>>>
  getPrimitiveArrayPropertyValues(
      const ClassProperty* classProperty,
      const FeatureTableProperty& featureTableProperty) const {
    if (classProperty->type != "ARRAY") {
      return std::nullopt;
    }

    if (!classProperty->componentType.isString()) {
      return std::nullopt;
    }

    PropertyType componentType =
        convertStringToPropertyType(classProperty->componentType.getString());
    if (TypeToPropertyType<T>::value != static_cast<uint32_t>(componentType)) {
      return std::nullopt;
    }

    gsl::span<const std::byte> valueBuffer =
        getBufferSafe(featureTableProperty.bufferView);
    if (valueBuffer.empty()) {
      return std::nullopt;
    }

    if (valueBuffer.size() % sizeof(T) != 0) {
      return std::nullopt;
    }

    int64_t componentCount = classProperty->componentCount.value_or(0);
    if (componentCount > 0 && featureTableProperty.arrayOffsetBufferView >= 0) {
      return std::nullopt;
    }

    if (componentCount <= 0 && featureTableProperty.arrayOffsetBufferView < 0) {
      return std::nullopt;
    }

    // fixed array
    if (componentCount > 0) {
      size_t maxRequiredBytes = 0;
      if constexpr (IsMetadataBoolean<T>::value) {
        maxRequiredBytes = static_cast<size_t>(glm::ceil(
            static_cast<double>(_featureTable->count * componentCount) / 8.0));
      } else {
        maxRequiredBytes = static_cast<size_t>(
            _featureTable->count * componentCount * sizeof(T));
      }

      if (valueBuffer.size() < maxRequiredBytes) {
        return std::nullopt;
      }

      return MetadataPropertyView<MetadataArrayView<T>>(
          valueBuffer,
          gsl::span<const std::byte>(),
          gsl::span<const std::byte>(),
          PropertyType::None,
          static_cast<size_t>(componentCount),
          static_cast<size_t>(_featureTable->count));
    }

    // dynamic array
    PropertyType offsetType =
        convertOffsetStringToPropertyType(featureTableProperty.offsetType);
    if (offsetType == PropertyType::None) {
      return std::nullopt;
    }

    constexpr bool checkBitsSize = IsMetadataBoolean<T>::value;
    gsl::span<const std::byte> offsetBuffer = getOffsetBufferSafe(
        featureTableProperty.arrayOffsetBufferView,
        offsetType,
        valueBuffer.size(),
        static_cast<size_t>(_featureTable->count),
        checkBitsSize);
    if (offsetBuffer.empty()) {
      return std::nullopt;
    }

    return MetadataPropertyView<MetadataArrayView<T>>(
        valueBuffer,
        offsetBuffer,
        gsl::span<const std::byte>(),
        offsetType,
        0,
        static_cast<size_t>(_featureTable->count));
  }

  std::optional<MetadataPropertyView<MetadataArrayView<std::string_view>>>
  getStringArrayPropertyValues(
      const ClassProperty* classProperty,
      const FeatureTableProperty& featureTableProperty) const;

  gsl::span<const std::byte> getBufferSafe(int32_t bufferViewIdx) const;

  gsl::span<const std::byte> getOffsetBufferSafe(
      int32_t bufferViewIdx,
      PropertyType offsetType,
      size_t valueBufferSize,
      size_t instanceCount,
      bool checkBitsSize) const;

  const Model* _model;
  const FeatureTable* _featureTable;
  const Class* _class;
};
} // namespace CesiumGltf
