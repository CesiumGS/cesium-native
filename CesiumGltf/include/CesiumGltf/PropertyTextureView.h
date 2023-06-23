#pragma once

#include "CesiumGltf/Class.h"
#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/PropertyTexture.h"
#include "CesiumGltf/PropertyTexturePropertyView.h"
#include "CesiumGltf/Texture.h"
#include "Model.h"

namespace CesiumGltf {
/**
 * @brief Indicates the status of a property texture view.
 *
 * The {@link PropertyTextureView} constructor always completes successfully.
 * However it may not always reflect the actual content of the
 * {@link PropertyTexture}. This enumeration provides the reason.
 */
enum class PropertyTextureViewStatus {
  /**
   * @brief This property texture view is valid and ready to use.
   */
  Valid,

  /**
   * @brief The glTF is missing the EXT_structural_metadata extension.
   */
  ErrorMissingMetadataExtension,

  /**
   * @brief The glTF EXT_structural_metadata extension doesn't contain a schema.
   */
  ErrorMissingSchema,

  /**
   * @brief The property texture's specified class could not be found in the
   * extension.
   */
  ErrorClassNotFound
};

/**
 * @brief A view on a {@link PropertyTexture}.
 *
 * This should be used to get a {@link PropertyTexturePropertyView} of a property in the property texture.
 * It will validate the EXT_structural_metadata format and ensure {@link PropertyTexturePropertyView}
 * does not access out of bounds.
 */
class PropertyTextureView {
public:
  /**
   * @brief Construct a PropertyTextureView.
   *
   * @param model The glTF that contains the property texture's data.
   * @param propertyTexture The {@link PropertyTexture}
   * from which the view will retrieve data.
   */
  PropertyTextureView(
      const Model& model,
      const PropertyTexture& propertyTexture) noexcept;

  /**
   * @brief Gets the status of this property texture view.
   *
   * Indicates whether the view accurately reflects the property texture's data,
   * or whether an error occurred.
   */
  PropertyTextureViewStatus status() const noexcept { return this->_status; }

  /**
   * @brief Finds the {@link ClassProperty} that
   * describes the type information of the property with the specified name.
   * @param propertyName The name of the property to retrieve the class for.
   * @return A pointer to the {@link ClassProperty}.
   * Return nullptr if the PropertyTextureView is invalid or if no class
   * property was found.
   */
  const ClassProperty* getClassProperty(const std::string& propertyName) const;

  /**
   * @brief Gets a {@link PropertyTexturePropertyView} that views the data of a
   * property stored in the {@link PropertyTexture}.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyTexturePropertyView} retrieves the correct data. T must be
   * a scalar with a supported component type (uint8_t, uint16_t, uint32_t,
   * float), or a glm vecN composed of one of the scalar types.
   * PropertyArrayViews are unsupported; if the property describes a
   * fixed-length array of scalars, T must be a glm vecN of the same length.
   *
   * @param propertyName The name of the property to retrieve data from
   * @return A {@link PropertyTablePropertyView} of the property. If no valid property is
   * found, the property view will be invalid.
   */
  template <typename T>
  PropertyTexturePropertyView<T>
  getPropertyView(const std::string& propertyName) const {
    if (this->_status != PropertyTextureViewStatus::Valid) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorInvalidPropertyTexture);
    }

    const ClassProperty* pClassProperty = getClassProperty(propertyName);
    if (!pClassProperty) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorNonexistentProperty);
    }

    return getPropertyViewImpl<T>(propertyName, *pClassProperty);
  }

private:
  template <typename T>
  PropertyTexturePropertyView<T> getPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty) const {
    auto propertyTexturePropertyIter =
        _pPropertyTexture->properties.find(propertyName);
    if (propertyTexturePropertyIter == _pPropertyTexture->properties.end()) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorNonexistentProperty);
    }

    const PropertyTextureProperty& propertyTextureProperty =
        propertyTexturePropertyIter->second;

    if constexpr (IsMetadataScalar<T>::value) {
      return createScalarPropertyView<T>(
          classProperty,
          propertyTextureProperty);
    } else if constexpr (IsMetadataVecN<T>::value) {
    } else if constexpr (IsMetadataArray<T>::value) {
      return createArrayPropertyView<typename MetadataArrayType<T>::type>(
          classProperty,
          propertyTextureProperty);
    } else {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }
  }

  template <typename T>
  PropertyTexturePropertyView<T> createScalarPropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& propertyTextureProperty) const {
    if (classProperty.array) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    // Eight-byte scalar types are unsupported.
    if constexpr (
        std::is_same_v<T, uint64_t> || std::is_same_v<T, int64_t> ||
        std::is_same_v<T, double>) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }

    return createPropertyViewImpl<T>(
        classProperty,
        propertyTextureProperty,
        sizeof(T));
  }

  template <typename T>
  PropertyTexturePropertyView<T> createVecNPropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& propertyTextureProperty) const {
    if (classProperty.array) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    // Only uint8 and uint16s are supported.
    if (componentType != PropertyComponentType::Uint8 &&
        componentType != PropertyComponentType::Uint16) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }

    // Only up to four bytes of image data are supported.
    if (sizeof(T) > 4) {
      return PropertyTexturePropertyView<T>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }

    return createPropertyViewImpl<T>(
        classProperty,
        propertyTextureProperty,
        sizeof(T));
  }

  template <typename T>
  PropertyTexturePropertyView<PropertyArrayView<T>> createArrayPropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& propertyTextureProperty) const {
    if (!classProperty.array) {
      return PropertyTexturePropertyView<PropertyArrayView<T>>(
          PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyTexturePropertyView<PropertyArrayView<T>>(
          PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
    }

    // Only scalar arrays are supported.
    if (type != PropertyType::Scalar) {
      return PropertyTexturePropertyView<PropertyArrayView<T>>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }

    // Only up to four elements are supported.
    int64_t count = classProperty.count.value_or(0);
    if (count <= 0 || count > 4) {
      return PropertyTexturePropertyView<PropertyArrayView<T>>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyTexturePropertyView<PropertyArrayView<T>>(
          PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    // Only uint8 and uint16s are supported.
    if (componentType != PropertyComponentType::Uint8 &&
        componentType != PropertyComponentType::Uint16) {
      return PropertyTexturePropertyView<PropertyArrayView<T>>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }

    if (componentType == PropertyComponentType::Uint16 && count > 2) {
      return PropertyTexturePropertyView<PropertyArrayView<T>>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }

    return createPropertyViewImpl<PropertyArrayView<T>>(
        classProperty,
        propertyTextureProperty,
        count * sizeof(T));
  }

  template <typename T>
  PropertyTexturePropertyView<T> createPropertyViewImpl(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& propertyTextureProperty,
      size_t elementSize) const {
    int32_t samplerIndex;
    int32_t imageIndex;

    auto status =
        getTextureSafe(propertyTextureProperty.index, samplerIndex, imageIndex);

    if (status != PropertyTexturePropertyViewStatus::Valid) {
      return PropertyTexturePropertyView<T>(status);
    }

    status = checkSampler(samplerIndex);
    if (status != PropertyTexturePropertyViewStatus::Valid) {
      return PropertyTexturePropertyView<T>(status);
    }

    status = checkImage(samplerIndex);
    if (status != PropertyTexturePropertyViewStatus::Valid) {
      return PropertyTexturePropertyView<T>(status);
    }

    const ImageCesium& image = _pModel->images[imageIndex].cesium;
    const std::vector<int64_t>& channels = propertyTextureProperty.channels;

    status = checkChannels(channels, image);
    if (status != PropertyTexturePropertyViewStatus::Valid) {
      return PropertyTexturePropertyView<T>(status);
    }

    if (channels.size() * image.bytesPerChannel != elementSize) {
      return PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch;
    }

    return PropertyTexturePropertyView<T>(
        _pModel->samplers[samplerIndex],
        image,
        propertyTextureProperty.texCoord,
        channels,
        classProperty.normalized);
  }

  PropertyTexturePropertyViewStatus getTextureSafe(
      const int32_t textureIndex,
      int32_t& samplerIndex,
      int32_t& imageIndex) const noexcept;

  PropertyTexturePropertyViewStatus
  checkSampler(const int32_t samplerIndex) const noexcept;

  PropertyTexturePropertyViewStatus
  checkImage(const int32_t imageIndex) const noexcept;

  PropertyTexturePropertyViewStatus checkChannels(
      const std::vector<int64_t>& channels,
      const ImageCesium& image) const noexcept;

  const Model* _pModel;
  const PropertyTexture* _pPropertyTexture;
  const Class* _pClass;

  PropertyTextureViewStatus _status;
};
} // namespace CesiumGltf
