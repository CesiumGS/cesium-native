#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/PropertyView.h"
#include <optional>

namespace CesiumGltf {
class FeatureTableView {
public:
  FeatureTableView(const Model* model, const FeatureTable* featureTable);

  const ClassProperty* getClassProperty(const std::string& propertyName) const;

  template <typename T>
  std::optional<PropertyView<T>>
  getPropertyValues(const std::string& propertyName) const {
    if (_featureTable->count < 0) {
      return std::nullopt;
    }

    const ClassProperty* classProperty = getClassProperty(propertyName);
    if (!classProperty) {
      return std::nullopt;
    }

    auto featureTablePropertyIter =
        _featureTable->properties.find(propertyName);
    if (featureTablePropertyIter == _featureTable->properties.end()) {
      return std::nullopt;
    }

    const FeatureTableProperty& featureTableProperty =
        featureTablePropertyIter->second;

    if constexpr (IsNumeric<T>::value) {
      return getNumericPropertyValues<T>(classProperty, featureTableProperty);
    }

    if constexpr (IsBoolean<T>::value) {
      return getBooleanPropertyValues(classProperty, featureTableProperty);
    }

    if constexpr (IsString<T>::value) {
      return getStringPropertyValues(classProperty, featureTableProperty);
    }

    if constexpr (IsNumericArray<T>::value) {
      return getNumericArrayPropertyValues<typename ArrayType<T>::type>(
          classProperty,
          featureTableProperty);
    }

    if constexpr (IsBooleanArray<T>::value) {
      return getBooleanArrayPropertyValues(classProperty, featureTableProperty);
    }

    if constexpr (IsStringArray<T>::value) {
      return getStringArrayPropertyValues(classProperty, featureTableProperty);
    }
  }

private:
  template <typename T>
  std::optional<PropertyView<T>> getNumericPropertyValues(
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

    if (valueBuffer.size() < _featureTable->count * sizeof(T)) {
      return std::nullopt;
    }

    return PropertyView<T>(
        valueBuffer,
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        PropertyType::None,
        0,
        _featureTable->count);
  }

  std::optional<PropertyView<bool>> getBooleanPropertyValues(
      const ClassProperty* classProperty,
      const FeatureTableProperty& featureTableProperty) const;

  std::optional<PropertyView<std::string_view>> getStringPropertyValues(
      const ClassProperty* classProperty,
      const FeatureTableProperty& featureTableProperty) const;

  template <typename T>
  std::optional<PropertyView<ArrayView<T>>> getNumericArrayPropertyValues(
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
      if (valueBuffer.size() !=
          static_cast<size_t>(
              _featureTable->count * componentCount * sizeof(T))) {
        return std::nullopt;
      }

      return PropertyView<ArrayView<T>>(
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

    gsl::span<const std::byte> offsetBuffer = getOffsetBufferSafe(
        featureTableProperty.arrayOffsetBufferView,
        offsetType,
        valueBuffer.size(),
        static_cast<size_t>(_featureTable->count),
        false);
    if (offsetBuffer.empty()) {
      return std::nullopt;
    }

    return PropertyView<ArrayView<T>>(
        valueBuffer,
        offsetBuffer,
        gsl::span<const std::byte>(),
        offsetType,
        0,
        static_cast<size_t>(_featureTable->count));
  }

  std::optional<PropertyView<ArrayView<bool>>> getBooleanArrayPropertyValues(
      const ClassProperty* classProperty,
      const FeatureTableProperty& featureTableProperty) const;

  std::optional<PropertyView<ArrayView<std::string_view>>>
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
