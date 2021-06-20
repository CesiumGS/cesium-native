#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/PropertyView.h"
#include "glm/common.hpp"
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

    if constexpr (IsNumeric<T>::value || IsBoolean<T>::value) {
      return getPrimitivePropertyValues<T>(classProperty, featureTableProperty);
    }

    if constexpr (IsString<T>::value) {
      return getStringPropertyValues(classProperty, featureTableProperty);
    }

    if constexpr (IsNumericArray<T>::value || IsBooleanArray<T>::value) {
      return getPrimitiveArrayPropertyValues<typename ArrayType<T>::type>(
          classProperty,
          featureTableProperty);
    }

    if constexpr (IsStringArray<T>::value) {
      return getStringArrayPropertyValues(classProperty, featureTableProperty);
    }
  }

private:
  template <typename T>
  std::optional<PropertyView<T>> getPrimitivePropertyValues(
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
    if (IsBoolean<T>::value) {
      maxRequiredBytes = static_cast<size_t>(
          glm::ceil(static_cast<double>(_featureTable->count) / 8.0));
    } else {
      maxRequiredBytes = _featureTable->count * sizeof(T);
    }

    if (valueBuffer.size() < maxRequiredBytes) {
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

  std::optional<PropertyView<std::string_view>> getStringPropertyValues(
      const ClassProperty* classProperty,
      const FeatureTableProperty& featureTableProperty) const;

  template <typename T>
  std::optional<PropertyView<ArrayView<T>>> getPrimitiveArrayPropertyValues(
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
      if constexpr (IsBoolean<T>::value) {
        maxRequiredBytes = static_cast<size_t>(glm::ceil(
            static_cast<double>(_featureTable->count * componentCount) / 8.0));
      } else {
        maxRequiredBytes = static_cast<size_t>(
            _featureTable->count * componentCount * sizeof(T));
      }

      if (valueBuffer.size() < maxRequiredBytes) {
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

    constexpr bool checkBitsSize = IsBoolean<T>::value;
    gsl::span<const std::byte> offsetBuffer = getOffsetBufferSafe(
        featureTableProperty.arrayOffsetBufferView,
        offsetType,
        valueBuffer.size(),
        static_cast<size_t>(_featureTable->count),
        checkBitsSize);
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
