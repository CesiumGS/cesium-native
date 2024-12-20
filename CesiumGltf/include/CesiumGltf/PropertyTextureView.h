#pragma once

#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyTexture.h>
#include <CesiumGltf/PropertyTexturePropertyView.h>
#include <CesiumGltf/TextureView.h>

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
 * This should be used to get a {@link PropertyTexturePropertyView} of a property
 * in the property texture. It will validate the EXT_structural_metadata format
 * and
 * ensure {@link PropertyTexturePropertyView} does not access data out of bounds.
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
   * @brief Gets the name of the property texture being viewed. Returns
   * std::nullopt if no name was specified.
   */
  const std::optional<std::string>& name() const noexcept {
    return _pPropertyTexture->name;
  }

  /**
   * @brief Gets the {@link Class} that this property texture conforms to.
   *
   * @return A pointer to the {@link Class}. Returns nullptr if the PropertyTexture
   * did not specify a valid class.
   */
  const Class* getClass() const noexcept { return _pClass; }

  /**
   * @brief Finds the {@link ClassProperty} that
   * describes the type information of the property with the specified id.
   * @param propertyId The id of the property to retrieve the class for.
   * @return A pointer to the {@link ClassProperty}.
   * Return nullptr if the PropertyTextureView is invalid or if no class
   * property was found.
   */
  const ClassProperty* getClassProperty(const std::string& propertyId) const;

  /**
   * @brief Gets a {@link PropertyTexturePropertyView} that views the data of a
   * property stored in the {@link PropertyTexture}.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyTexturePropertyView} retrieves the correct data. T must
   * be a scalar with a supported component type (int8_t, uint8_t, int16_t,
   * uint16_t, int32_t, uint32_t, float), a glm vecN composed of one of the
   * scalar types, or a PropertyArrayView containing one of the scalar types.
   *
   * If T does not match the type specified by the class property, this returns
   * an invalid PropertyTexturePropertyView. Likewise, if the value of
   * Normalized does not match the value of {@link ClassProperty::normalized} for that
   * class property, this returns an invalid property view. Only types with
   * integer components may be normalized.
   *
   * @tparam T The C++ type corresponding to the type of the data retrieved.
   * @tparam Normalized Whether the property is normalized. Only applicable to
   * types with integer components.
   * @param propertyId The ID of the property to retrieve data from
   * @param propertyOptions The options to apply to the property.
   * @return A {@link PropertyTexturePropertyView} of the property. If no valid
   * property is found, the property view will be invalid.
   */
  template <typename T, bool Normalized = false>
  PropertyTexturePropertyView<T, Normalized> getPropertyView(
      const std::string& propertyId,
      const TextureViewOptions& propertyOptions = TextureViewOptions()) const {
    if (this->_status != PropertyTextureViewStatus::Valid) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorInvalidPropertyTexture);
    }

    const ClassProperty* pClassProperty = getClassProperty(propertyId);
    if (!pClassProperty) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorNonexistentProperty);
    }

    return getPropertyViewImpl<T, Normalized>(
        propertyId,
        *pClassProperty,
        propertyOptions);
  }

  /**
   * @brief Gets a {@link PropertyTexturePropertyView} through a callback that accepts a
   * property id and a {@link PropertyTexturePropertyView} that views the data
   * of the property with the specified id.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyTexturePropertyView} retrieves the correct data. T must
   * be a scalar with a supported component type (int8_t, uint8_t, int16_t,
   * uint16_t, int32_t, uint32_t, float), a glm vecN composed of one of the
   * scalar types, or a PropertyArrayView containing one of the scalar types.
   *
   * If the property is somehow invalid, an empty {@link PropertyTexturePropertyView}
   * with an error status will be passed to the callback. Otherwise, a valid
   * property view will be passed to the callback.
   *
   * @param propertyId The id of the property to retrieve data from
   * @param callback A callback function that accepts a property id and a
   * {@link PropertyTexturePropertyView}
   * @param propertyOptions The options to apply to the property.
   * @tparam Callback The type of the callback function.
   */
  template <typename Callback>
  void getPropertyView(
      const std::string& propertyId,
      Callback&& callback,
      const TextureViewOptions& propertyOptions = TextureViewOptions()) const {
    if (this->_status != PropertyTextureViewStatus::Valid) {
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorInvalidPropertyTexture));
      return;
    }

    const ClassProperty* pClassProperty = getClassProperty(propertyId);
    if (!pClassProperty) {
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorNonexistentProperty));
      return;
    }

    PropertyType type = convertStringToPropertyType(pClassProperty->type);
    PropertyComponentType componentType = PropertyComponentType::None;
    if (pClassProperty->componentType) {
      componentType =
          convertStringToPropertyComponentType(*pClassProperty->componentType);
    }

    bool normalized = pClassProperty->normalized;
    if (normalized && !isPropertyComponentTypeInteger(componentType)) {
      // Only integer components may be normalized.
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorInvalidNormalization));
      return;
    }

    if (pClassProperty->array) {
      if (normalized) {
        getArrayPropertyViewImpl<Callback, true>(
            propertyId,
            *pClassProperty,
            type,
            componentType,
            std::forward<Callback>(callback),
            propertyOptions);
      } else {
        getArrayPropertyViewImpl<Callback, false>(
            propertyId,
            *pClassProperty,
            type,
            componentType,
            std::forward<Callback>(callback),
            propertyOptions);
      }
      return;
    }

    if (type == PropertyType::Scalar) {
      if (normalized) {
        getScalarPropertyViewImpl<Callback, true>(
            propertyId,
            *pClassProperty,
            componentType,
            std::forward<Callback>(callback),
            propertyOptions);
      } else {
        getScalarPropertyViewImpl<Callback, false>(
            propertyId,
            *pClassProperty,
            componentType,
            std::forward<Callback>(callback),
            propertyOptions);
      }
      return;
    }

    if (isPropertyTypeVecN(type)) {
      if (normalized) {
        getVecNPropertyViewImpl<Callback, true>(
            propertyId,
            *pClassProperty,
            type,
            componentType,
            std::forward<Callback>(callback),
            propertyOptions);
      } else {
        getVecNPropertyViewImpl<Callback, false>(
            propertyId,
            *pClassProperty,
            type,
            componentType,
            std::forward<Callback>(callback),
            propertyOptions);
      }
      return;
    }

    callback(
        propertyId,
        PropertyTexturePropertyView<uint8_t>(
            PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty));
    return;
  }

  /**
   * @brief Iterates over each property in the {@link PropertyTexture} with a callback
   * that accepts a property id and a {@link PropertyTexturePropertyView} to view
   * the data stored in the {@link PropertyTextureProperty}.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyTexturePropertyView} retrieves the correct data. T must be
   * a scalar with a supported component type (int8_t, uint8_t, int16_t,
   * uint16_t, int32_t, uint32_t, float), a glm vecN composed of one of the
   * scalar types, or a PropertyArrayView containing one of the scalar types.
   *
   * If the property is invalid, an empty {@link PropertyTexturePropertyView} with an
   * error status will be passed to the callback. Otherwise, a valid property
   * view will be passed to the callback.
   *
   * @param callback A callback function that accepts property id and
   * {@link PropertyTexturePropertyView}
   * @param propertyOptions The options to apply to each property in the
   * property texture.
   * @tparam Callback The type of the callback function.
   */

  template <typename Callback>
  void forEachProperty(
      Callback&& callback,
      const TextureViewOptions& propertyOptions = TextureViewOptions()) const {
    for (const auto& property : this->_pClass->properties) {
      getPropertyView(
          property.first,
          std::forward<Callback>(callback),
          propertyOptions);
    }
  }

private:
  template <typename T, bool Normalized>
  PropertyTexturePropertyView<T, Normalized> getPropertyViewImpl(
      const std::string& propertyId,
      const ClassProperty& classProperty,
      const TextureViewOptions& propertyOptions) const {
    auto propertyTexturePropertyIter =
        _pPropertyTexture->properties.find(propertyId);
    if (propertyTexturePropertyIter == _pPropertyTexture->properties.end()) {
      if (!classProperty.required && classProperty.defaultProperty) {
        // If the property was omitted from the property texture, it is still
        // technically valid if it specifies a default value. Create a view that
        // just returns the default value.
        return PropertyTexturePropertyView<T, Normalized>(classProperty);
      }

      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorNonexistentProperty);
    }

    const PropertyTextureProperty& propertyTextureProperty =
        propertyTexturePropertyIter->second;

    if constexpr (IsMetadataScalar<T>::value) {
      return createScalarPropertyView<T, Normalized>(
          classProperty,
          propertyTextureProperty,
          propertyOptions);
    }

    if constexpr (IsMetadataVecN<T>::value) {
      return createVecNPropertyView<T, Normalized>(
          classProperty,
          propertyTextureProperty,
          propertyOptions);
    }

    if constexpr (IsMetadataArray<T>::value) {
      return createArrayPropertyView<
          typename MetadataArrayType<T>::type,
          Normalized>(classProperty, propertyTextureProperty, propertyOptions);
    }
  }

  template <typename Callback, bool Normalized>
  void getArrayPropertyViewImpl(
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback,
      const TextureViewOptions& propertyOptions) const {
    // Only scalar arrays are supported.
    if (type != PropertyType::Scalar) {
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty));
      return;
    }

    int64_t count = classProperty.count.value_or(0);
    if (count <= 0 || count > 4) {
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty));
      return;
    }

    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyId,
          getPropertyViewImpl<PropertyArrayView<int8_t>, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyId,
          getPropertyViewImpl<PropertyArrayView<uint8_t>, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyId,
          getPropertyViewImpl<PropertyArrayView<int16_t>, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyId,
          getPropertyViewImpl<PropertyArrayView<uint16_t>, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    default:
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty));
      break;
    }
  }

  template <typename Callback, bool Normalized>
  void getScalarPropertyViewImpl(
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback,
      const TextureViewOptions& propertyOptions) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyId,
          getPropertyViewImpl<int8_t, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      return;
    case PropertyComponentType::Uint8:
      callback(
          propertyId,
          getPropertyViewImpl<uint8_t, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      return;
    case PropertyComponentType::Int16:
      callback(
          propertyId,
          getPropertyViewImpl<int16_t, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      return;
    case PropertyComponentType::Uint16:
      callback(
          propertyId,
          getPropertyViewImpl<uint16_t, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyId,
          getPropertyViewImpl<int32_t, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyId,
          getPropertyViewImpl<uint32_t, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyId,
          getPropertyViewImpl<float, false>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    default:
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty));
      break;
    }
  }

  template <typename Callback, glm::length_t N, bool Normalized>
  void getVecNPropertyViewImpl(
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback,
      const TextureViewOptions& propertyOptions) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyId,
          getPropertyViewImpl<glm::vec<N, int8_t>, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyId,
          getPropertyViewImpl<glm::vec<N, uint8_t>, Normalized>(
              propertyId,
              classProperty,
              propertyOptions));
      break;
    case PropertyComponentType::Int16:
      if constexpr (N == 2) {
        callback(
            propertyId,
            getPropertyViewImpl<glm::vec<N, int16_t>, Normalized>(
                propertyId,
                classProperty,
                propertyOptions));
        break;
      }
      [[fallthrough]];
    case PropertyComponentType::Uint16:
      if constexpr (N == 2) {
        callback(
            propertyId,
            getPropertyViewImpl<glm::vec<N, uint16_t>, Normalized>(
                propertyId,
                classProperty,
                propertyOptions));
        break;
      }
      [[fallthrough]];
    default:
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty));
      break;
    }
  }

  template <typename Callback, bool Normalized>
  void getVecNPropertyViewImpl(
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback,
      const TextureViewOptions& propertyOptions) const {
    const glm::length_t N = getDimensionsFromPropertyType(type);
    switch (N) {
    case 2:
      getVecNPropertyViewImpl<Callback, 2, Normalized>(
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback),
          propertyOptions);
      break;
    case 3:
      getVecNPropertyViewImpl<Callback, 3, Normalized>(
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback),
          propertyOptions);
      break;
    case 4:
      getVecNPropertyViewImpl<Callback, 4, Normalized>(
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback),
          propertyOptions);
      break;
    default:
      callback(
          propertyId,
          PropertyTexturePropertyView<uint8_t>(
              PropertyTexturePropertyViewStatus::ErrorTypeMismatch));
      break;
    }
  }

  template <typename T, bool Normalized>
  PropertyTexturePropertyView<T, Normalized> createScalarPropertyView(
      const ClassProperty& classProperty,
      [[maybe_unused]] const PropertyTextureProperty& propertyTextureProperty,
      const TextureViewOptions& propertyOptions) const {
    if (classProperty.array) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    if (classProperty.normalized != Normalized) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
    }

    // Only up to four bytes of image data are supported.
    if constexpr (sizeof(T) <= 4) {
      return createPropertyViewImpl<T, Normalized>(
          classProperty,
          propertyTextureProperty,
          sizeof(T),
          propertyOptions);
    } else {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }
  }

  template <typename T, bool Normalized>
  PropertyTexturePropertyView<T, Normalized> createVecNPropertyView(
      const ClassProperty& classProperty,
      [[maybe_unused]] const PropertyTextureProperty& propertyTextureProperty,
      [[maybe_unused]] const TextureViewOptions& propertyOptions) const {
    if (classProperty.array) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    if (classProperty.normalized != Normalized) {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
    }

    // Only up to four bytes of image data are supported.
    if constexpr (sizeof(T) <= 4) {
      return createPropertyViewImpl<T, Normalized>(
          classProperty,
          propertyTextureProperty,
          sizeof(T),
          propertyOptions);
    } else {
      return PropertyTexturePropertyView<T, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }
  }

  template <typename T, bool Normalized>
  PropertyTexturePropertyView<PropertyArrayView<T>, Normalized>
  createArrayPropertyView(
      const ClassProperty& classProperty,
      [[maybe_unused]] const PropertyTextureProperty& propertyTextureProperty,
      [[maybe_unused]] const TextureViewOptions& propertyOptions) const {
    if (!classProperty.array) {
      return PropertyTexturePropertyView<PropertyArrayView<T>, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyTexturePropertyView<PropertyArrayView<T>, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyTexturePropertyView<PropertyArrayView<T>, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    if (classProperty.normalized != Normalized) {
      return PropertyTexturePropertyView<PropertyArrayView<T>, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
    }

    // Only scalar arrays are supported. The scalar component type must not
    // exceed two bytes.
    if constexpr (IsMetadataScalar<T>::value && sizeof(T) <= 4) {
      // Only up to four elements are supported.
      int64_t count = classProperty.count.value_or(0);
      if (count <= 0 || count > 4) {
        return PropertyTexturePropertyView<PropertyArrayView<T>, Normalized>(
            PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
      }

      if (count * sizeof(T) > 4) {
        return PropertyTexturePropertyView<PropertyArrayView<T>, Normalized>(
            PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
      }

      return createPropertyViewImpl<PropertyArrayView<T>, Normalized>(
          classProperty,
          propertyTextureProperty,
          count * sizeof(T),
          propertyOptions);
    } else {
      return PropertyTexturePropertyView<PropertyArrayView<T>, Normalized>(
          PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
    }
  }

  template <typename T, bool Normalized>
  PropertyTexturePropertyView<T, Normalized> createPropertyViewImpl(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& propertyTextureProperty,
      size_t elementSize,
      const TextureViewOptions& propertyOptions) const {
    int32_t samplerIndex;
    int32_t imageIndex;

    auto status =
        getTextureSafe(propertyTextureProperty.index, samplerIndex, imageIndex);

    if (status != PropertyTexturePropertyViewStatus::Valid) {
      return PropertyTexturePropertyView<T, Normalized>(status);
    }

    status = checkSampler(samplerIndex);
    if (status != PropertyTexturePropertyViewStatus::Valid) {
      return PropertyTexturePropertyView<T, Normalized>(status);
    }

    status = checkImage(imageIndex);
    if (status != PropertyTexturePropertyViewStatus::Valid) {
      return PropertyTexturePropertyView<T, Normalized>(status);
    }

    const CesiumUtility::IntrusivePointer<ImageAsset>& pImage =
        _pModel->images[imageIndex].pAsset;
    const std::vector<int64_t>& channels = propertyTextureProperty.channels;

    status = checkChannels(channels, *pImage);
    if (status != PropertyTexturePropertyViewStatus::Valid) {
      return PropertyTexturePropertyView<T, Normalized>(status);
    }

    if (channels.size() * pImage->bytesPerChannel != elementSize) {
      return PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch;
    }

    return PropertyTexturePropertyView<T, Normalized>(
        propertyTextureProperty,
        classProperty,
        _pModel->samplers[samplerIndex],
        *pImage,
        propertyOptions);
  }

  PropertyViewStatusType getTextureSafe(
      const int32_t textureIndex,
      int32_t& samplerIndex,
      int32_t& imageIndex) const noexcept;

  PropertyViewStatusType
  checkSampler(const int32_t samplerIndex) const noexcept;

  PropertyViewStatusType checkImage(const int32_t imageIndex) const noexcept;

  PropertyViewStatusType checkChannels(
      const std::vector<int64_t>& channels,
      const ImageAsset& image) const noexcept;

  const Model* _pModel;
  const PropertyTexture* _pPropertyTexture;
  const Class* _pClass;

  PropertyTextureViewStatus _status;
};
} // namespace CesiumGltf
