#pragma once

#include "CesiumGltf/ExtensionModelExtFeatureMetadata.h"
#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/PropertyType.h"

#include <glm/common.hpp>

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
      const Model* pModel,
      const FeatureTable* pFeatureTable);

  /**
   * @brief Find the {@link ClassProperty} which stores the type information of a property based on the property name
   * @param propertyName The name of the property to retrieve type info
   * @return ClassProperty of a property. Return nullptr if no property is found
   */
  const ClassProperty* getClassProperty(const std::string& propertyName) const;

  /**
   * @brief Get MetadataPropertyView to view the data of a property stored in
   * the FeatureTable.
   *
   * This method will validate the EXT_Feature_Metadata format to ensure
   * MetadataPropertyView retrieve the correct data. T must be uin8_t, int8_t,
   * uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, float, double,
   * bool, std::string_view, and MetadataArrayView<T> with T must be one of the
   * types mentioned above
   *
   * @param propertyName The name of the property to retrieve data from
   * @return ClassProperty of a property. Return nullptr if no property is found
   */
  template <typename T>
  MetadataPropertyView<T>
  getPropertyView(const std::string& propertyName) const {
    if (_pFeatureTable->count < 0) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::InvalidPropertyNotExist);
    }

    const ClassProperty* pClassProperty = getClassProperty(propertyName);
    if (!pClassProperty) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::InvalidPropertyNotExist);
    }

    return getPropertyViewImpl<T>(propertyName, *pClassProperty);
  }

  /**
   * @brief Get MetadataPropertyView through a callback that accepts property
   * name and std::optional<MetadataPropertyView<T>> to view the data of a
   * property stored in the FeatureTable.
   *
   * This method will validate the EXT_Feature_Metadata format to ensure
   * MetadataPropertyView retrieve the correct data. T must be uin8_t, int8_t,
   * uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, float, double,
   * bool, std::string_view, and MetadataArrayView<T> with T must be one of the
   * types mentioned above. If the property is invalid, std::nullopt will be
   * passed to the callback. Otherwise, a valid property view will be passed to
   * the callback
   *
   * @param propertyName The name of the property to retrieve data from
   * @tparam callback A callback function that accepts property name and
   * std::optional<MetadataPropertyView<T>>
   */
  template <typename Callback>
  void
  getPropertyView(const std::string& propertyName, Callback&& callback) const {
    const ClassProperty* pClassProperty = getClassProperty(propertyName);
    if (!pClassProperty) {
      return;
    }

    PropertyType type = convertStringToPropertyType(pClassProperty->type);
    PropertyType componentType = PropertyType::None;
    if (pClassProperty->componentType.has_value()) {
      componentType =
          convertStringToPropertyType(pClassProperty->componentType.value());
    }

    if (type != PropertyType::Array) {
      getScalarPropertyViewImpl(
          propertyName,
          *pClassProperty,
          type,
          std::forward<Callback>(callback));
    } else {
      getArrayPropertyViewImpl(
          propertyName,
          *pClassProperty,
          componentType,
          std::forward<Callback>(callback));
    }
  }

  /**
   * @brief Get MetadataPropertyView for each property in the FeatureTable
   * through a callback that accepts property name and
   * std::optional<MetadataPropertyView<T>> to view the data stored in the
   * FeatureTableProperty.
   *
   * This method will validate the EXT_Feature_Metadata format to ensure
   * MetadataPropertyView retrieve the correct data. T must be uin8_t, int8_t,
   * uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, float, double,
   * bool, std::string_view, and MetadataArrayView<T> with T must be one of the
   * types mentioned above. If the property is invalid, std::nullopt will be
   * passed to the callback. Otherwise, a valid property view will be passed to
   * the callback
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
  template <typename Callback>
  void getArrayPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
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
      const ClassProperty& classProperty,
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
  MetadataPropertyView<T> getPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty) const {
    auto featureTablePropertyIter =
        _pFeatureTable->properties.find(propertyName);
    if (featureTablePropertyIter == _pFeatureTable->properties.end()) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::InvalidPropertyNotExist);
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
  MetadataPropertyView<T> getPrimitivePropertyValues(
      const ClassProperty& classProperty,
      const FeatureTableProperty& featureTableProperty) const {
    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::InvalidTypeMismatch);
    }

    gsl::span<const std::byte> valueBuffer;
    const auto status =
        getBufferSafe(featureTableProperty.bufferView, valueBuffer);
    if (status != MetadataPropertyViewStatus::Valid) {
      return createInvalidPropertyView<T>(status);
    }

    if (valueBuffer.size() % sizeof(T) != 0) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::
              InvalidBufferViewSizeNotDivisibleByTypeSize);
    }

    size_t maxRequiredBytes = 0;
    if (IsMetadataBoolean<T>::value) {
      maxRequiredBytes = static_cast<size_t>(
          glm::ceil(static_cast<double>(_pFeatureTable->count) / 8.0));
    } else {
      maxRequiredBytes = _pFeatureTable->count * sizeof(T);
    }

    if (valueBuffer.size() < maxRequiredBytes) {
      return createInvalidPropertyView<T>(
          MetadataPropertyViewStatus::InvalidBufferViewSizeNotFitInstanceCount);
    }

    return MetadataPropertyView<T>(
        MetadataPropertyViewStatus::Valid,
        valueBuffer,
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        PropertyType::None,
        0,
        _pFeatureTable->count,
        classProperty.normalized);
  }

  MetadataPropertyView<std::string_view> getStringPropertyValues(
      const ClassProperty& classProperty,
      const FeatureTableProperty& featureTableProperty) const;

  template <typename T>
  MetadataPropertyView<MetadataArrayView<T>> getPrimitiveArrayPropertyValues(
      const ClassProperty& classProperty,
      const FeatureTableProperty& featureTableProperty) const {
    if (classProperty.type != ClassProperty::Type::ARRAY) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::InvalidTypeMismatch);
    }

    if (!classProperty.componentType.has_value()) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::InvalidTypeMismatch);
    }

    const PropertyType componentType =
        convertStringToPropertyType(classProperty.componentType.value());
    if (TypeToPropertyType<T>::value != componentType) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::InvalidTypeMismatch);
    }

    gsl::span<const std::byte> valueBuffer;
    auto status = getBufferSafe(featureTableProperty.bufferView, valueBuffer);
    if (status != MetadataPropertyViewStatus::Valid) {
      return createInvalidPropertyView<MetadataArrayView<T>>(status);
    }

    if (valueBuffer.size() % sizeof(T) != 0) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::
              InvalidBufferViewSizeNotDivisibleByTypeSize);
    }

    const int64_t componentCount = classProperty.componentCount.value_or(0);
    if (componentCount > 0 && featureTableProperty.arrayOffsetBufferView >= 0) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::
              InvalidArrayComponentCountAndOffsetBufferCoexist);
    }

    if (componentCount <= 0 && featureTableProperty.arrayOffsetBufferView < 0) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::
              InvalidArrayComponentCountOrOffsetBufferNotExist);
    }

    // fixed array
    if (componentCount > 0) {
      size_t maxRequiredBytes = 0;
      if constexpr (IsMetadataBoolean<T>::value) {
        maxRequiredBytes = static_cast<size_t>(glm::ceil(
            static_cast<double>(_pFeatureTable->count * componentCount) / 8.0));
      } else {
        maxRequiredBytes = static_cast<size_t>(
            _pFeatureTable->count * componentCount * sizeof(T));
      }

      if (valueBuffer.size() < maxRequiredBytes) {
        return createInvalidPropertyView<MetadataArrayView<T>>(
            MetadataPropertyViewStatus::
                InvalidBufferViewSizeNotFitInstanceCount);
      }

      return MetadataPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::Valid,
          valueBuffer,
          gsl::span<const std::byte>(),
          gsl::span<const std::byte>(),
          PropertyType::None,
          static_cast<size_t>(componentCount),
          static_cast<size_t>(_pFeatureTable->count),
          classProperty.normalized);
    }

    // dynamic array
    const PropertyType offsetType =
        convertOffsetStringToPropertyType(featureTableProperty.offsetType);
    if (offsetType == PropertyType::None) {
      return createInvalidPropertyView<MetadataArrayView<T>>(
          MetadataPropertyViewStatus::InvalidOffsetType);
    }

    constexpr bool checkBitsSize = IsMetadataBoolean<T>::value;
    gsl::span<const std::byte> offsetBuffer;
    status = getOffsetBufferSafe(
        featureTableProperty.arrayOffsetBufferView,
        offsetType,
        valueBuffer.size(),
        static_cast<size_t>(_pFeatureTable->count),
        checkBitsSize,
        offsetBuffer);
    if (status != MetadataPropertyViewStatus::Valid) {
      return createInvalidPropertyView<MetadataArrayView<T>>(status);
    }

    return MetadataPropertyView<MetadataArrayView<T>>(
        MetadataPropertyViewStatus::Valid,
        valueBuffer,
        offsetBuffer,
        gsl::span<const std::byte>(),
        offsetType,
        0,
        static_cast<size_t>(_pFeatureTable->count),
        classProperty.normalized);
  }

  MetadataPropertyView<MetadataArrayView<std::string_view>>
  getStringArrayPropertyValues(
      const ClassProperty& classProperty,
      const FeatureTableProperty& featureTableProperty) const;

  MetadataPropertyViewStatus getBufferSafe(
      int32_t bufferViewIdx,
      gsl::span<const std::byte>& buffer) const noexcept;

  MetadataPropertyViewStatus getOffsetBufferSafe(
      int32_t bufferViewIdx,
      PropertyType offsetType,
      size_t valueBufferSize,
      size_t instanceCount,
      bool checkBitsSize,
      gsl::span<const std::byte>& offsetBuffer) const noexcept;

  template <typename T>
  static MetadataPropertyView<T>
  createInvalidPropertyView(MetadataPropertyViewStatus invalidStatus) noexcept {
    return MetadataPropertyView<T>(
        invalidStatus,
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        PropertyType::None,
        0,
        0,
        false);
  }

  const Model* _pModel;
  const FeatureTable* _pFeatureTable;
  const Class* _pClass;
};
} // namespace CesiumGltf
