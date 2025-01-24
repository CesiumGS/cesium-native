#pragma once

#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyAttribute.h>
#include <CesiumGltf/PropertyAttributePropertyView.h>

namespace CesiumGltf {
/**
 * @brief Indicates the status of a property attribute view.
 *
 * The {@link PropertyAttributeView} constructor always completes successfully.
 * However it may not always reflect the actual content of the
 * {@link PropertyAttribute}. This enumeration provides the reason.
 */
enum class PropertyAttributeViewStatus {
  /**
   * @brief This property attribute view is valid and ready to use.
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
   * @brief The property attribute's specified class could not be found in the
   * extension.
   */
  ErrorClassNotFound
};

/**
 * @brief Attempts to obtain a \ref PropertyType from the \ref Accessor::type
 * "type" field of the accessor.
 *
 * @param accessor The accessor whose type will be obtained.
 * @returns A \ref PropertyType equivalent to the accessor's \ref
 * AccessorSpec::type, or \ref PropertyType::Invalid if no conversion could be
 * made.
 */
PropertyType getAccessorTypeAsPropertyType(const Accessor& accessor);

/**
 * @brief Attempts to obtain a \ref PropertyComponentType from the \ref
 * Accessor::componentType "componentType" field of the accessor.
 *
 * @param accessor The accessor whose componentType will be obtained.
 * @returns A \ref PropertyComponentType equivalent to the accessor's \ref
 * AccessorSpec::componentType, or \ref PropertyComponentType::None if no
 * conversion could be made.
 */
PropertyComponentType
getAccessorComponentTypeAsPropertyComponentType(const Accessor& accessor);

/**
 * @brief A view on a {@link PropertyAttribute}.
 *
 * This should be used to get a {@link PropertyAttributePropertyView} of a property
 * in the property attribute. It will validate the EXT_structural_metadata
 * format and ensure {@link PropertyAttributePropertyView} does not access data out
 *  of bounds.
 */
class PropertyAttributeView {
public:
  /**
   * @brief Construct a PropertyAttributeView.
   *
   * @param model The glTF that contains the property attribute's data.
   * @param propertyAttribute The {@link PropertyAttribute} from which
   * the view will retrieve data.
   */
  PropertyAttributeView(
      const Model& model,
      const PropertyAttribute& propertyAttribute) noexcept;

  /**
   * @brief Gets the status of this property attribute view.
   *
   * Indicates whether the view accurately reflects the property attribute's
   * data, or whether an error occurred.
   */
  PropertyAttributeViewStatus status() const noexcept { return this->_status; }

  /**
   * @brief Gets the name of the property attribute being viewed. Returns
   * std::nullopt if no name was specified.
   */
  const std::optional<std::string>& name() const noexcept {
    return _pPropertyAttribute->name;
  }

  /**
   * @brief Gets the {@link Class} that this property attribute conforms to.
   *
   * @return A pointer to the {@link Class}. Returns nullptr if the
   * PropertyAttribute did not specify a valid class.
   */
  const Class* getClass() const noexcept { return _pClass; }

  /**
   * @brief Finds the {@link ClassProperty} that
   * describes the type information of the property with the specified id.
   * @param propertyId The id of the property to retrieve the class for.
   * @return A pointer to the {@link ClassProperty}.
   * Return nullptr if the PropertyAttributeView is invalid or if no class
   * property was found.
   */
  const ClassProperty* getClassProperty(const std::string& propertyId) const;

  /**
   * @brief Gets a {@link PropertyAttributePropertyView} that views the data of a
   * property stored in the {@link PropertyAttribute}.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyAttributePropertyView} retrieves the correct data. T must
   * be a scalar with a supported component type (int8_t, uint8_t, int16_t,
   * uint16_t, float), a glm vecN composed of one of the scalar types,
   * or a glm matN containing one of the scalar types.
   *
   * If T does not match the type specified by the class property, this returns
   * an invalid PropertyAttributePropertyView. Likewise, if the value of
   * Normalized does not match the value of {@link ClassProperty::normalized} for that
   * class property, this returns an invalid property view. Only types with
   * integer components may be normalized.
   *
   * @tparam T The C++ type corresponding to the type of the data retrieved.
   * @tparam Normalized Whether the property is normalized. Only applicable to
   * types with integer components.
   * @param primitive The target primitive
   * @param propertyId The id of the property to retrieve data from
   * @return A {@link PropertyAttributePropertyView} of the property. If no valid
   * property is found, the property view will be invalid.
   */
  template <typename T, bool Normalized = false>
  PropertyAttributePropertyView<T, Normalized> getPropertyView(
      const MeshPrimitive& primitive,
      const std::string& propertyId) const {
    if (this->_status != PropertyAttributeViewStatus::Valid) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorInvalidPropertyAttribute);
    }
    const ClassProperty* pClassProperty = getClassProperty(propertyId);
    if (!pClassProperty) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorNonexistentProperty);
    }

    if constexpr (
        IsMetadataArray<T>::value || IsMetadataBoolean<T>::value ||
        IsMetadataString<T>::value) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty);
    }

    return getPropertyViewImpl<T, Normalized>(
        primitive,
        propertyId,
        *pClassProperty);
  }

  /**
   * @brief Gets a {@link PropertyAttributePropertyView} through a callback that accepts a
   * property id and a {@link PropertyAttributePropertyView} that views the data
   * of the property with the specified id.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyAttributePropertyView} retrieves the correct data. T must
   * be a scalar with a supported component type (int8_t, uint8_t, int16_t,
   * uint16_t, float), a glm vecN composed of one of the scalar types,
   * or a glm matN containing one of the scalar types.
   *
   * If the property is somehow invalid, an empty {@link PropertyAttributePropertyView}
   * with an error status will be passed to the callback. Otherwise, a valid
   * property view will be passed to the callback.
   *
   * @param primitive The target primitive
   * @param propertyId The id of the property to retrieve data from
   * @param callback A callback function that accepts a property id and a
   * {@link PropertyAttributePropertyView}
   * @tparam Callback The type of the callback function.
   */
  template <typename Callback>
  void getPropertyView(
      const MeshPrimitive& primitive,
      const std::string& propertyId,
      Callback&& callback) const {
    if (this->_status != PropertyAttributeViewStatus::Valid) {
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::
                  ErrorInvalidPropertyAttribute));
      return;
    }

    const ClassProperty* pClassProperty = getClassProperty(propertyId);
    if (!pClassProperty) {
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::ErrorNonexistentProperty));
      return;
    }

    if (pClassProperty->array) {
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty));
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
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::ErrorInvalidNormalization));
      return;
    }

    if (type == PropertyType::Scalar) {
      if (normalized) {
        getScalarPropertyViewImpl<Callback, true>(
            primitive,
            propertyId,
            *pClassProperty,
            componentType,
            std::forward<Callback>(callback));
      } else {
        getScalarPropertyViewImpl<Callback, false>(
            primitive,
            propertyId,
            *pClassProperty,
            componentType,
            std::forward<Callback>(callback));
      }
      return;
    }

    if (isPropertyTypeVecN(type)) {
      if (normalized) {
        getVecNPropertyViewImpl<Callback, true>(
            primitive,
            propertyId,
            *pClassProperty,
            type,
            componentType,
            std::forward<Callback>(callback));
      } else {
        getVecNPropertyViewImpl<Callback, false>(
            primitive,
            propertyId,
            *pClassProperty,
            type,
            componentType,
            std::forward<Callback>(callback));
      }
      return;
    }

    if (isPropertyTypeMatN(type)) {
      if (normalized) {
        getMatNPropertyViewImpl<Callback, true>(
            primitive,
            propertyId,
            *pClassProperty,
            type,
            componentType,
            std::forward<Callback>(callback));
      } else {
        getMatNPropertyViewImpl<Callback, false>(
            primitive,
            propertyId,
            *pClassProperty,
            type,
            componentType,
            std::forward<Callback>(callback));
      }
      return;
    }

    callback(
        propertyId,
        PropertyAttributePropertyView<uint8_t>(
            PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty));
    return;
  }

  /**
   * @brief Iterates over each property in the {@link PropertyAttribute} with a callback
   * that accepts a property id and a {@link PropertyAttributePropertyView} to view
   * the data stored in the {@link PropertyAttributeProperty}.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyAttributePropertyView} retrieves the correct data. T must be
   * a scalar with a supported component type (int8_t, uint8_t, int16_t,
   * uint16_t, float), a glm vecN composed of one of the scalar types,
   * or a PropertyArrayView containing one of the scalar types.
   *
   * If the property is invalid, an empty {@link PropertyAttributePropertyView} with an
   * error status will be passed to the callback. Otherwise, a valid property
   * view will be passed to the callback.
   *
   * @param primitive The id of the property to retrieve data from
   * @param callback A callback function that accepts property id and
   * {@link PropertyAttributePropertyView}
   * @tparam Callback The type of the callback function.
   */
  template <typename Callback>
  void
  forEachProperty(const MeshPrimitive& primitive, Callback&& callback) const {
    for (const auto& property : this->_pClass->properties) {
      getPropertyView(
          primitive,
          property.first,
          std::forward<Callback>(callback));
    }
  }

private:
  template <typename T, bool Normalized>
  PropertyAttributePropertyView<T, Normalized> getEmptyPropertyViewWithDefault(
      const MeshPrimitive& primitive,
      const ClassProperty& classProperty) const {
    // To make the view have a nonzero size, find the POSITION attribute and get
    // its accessor count. If it doesn't exist or is somehow erroneous, just
    // mark the property as nonexistent.
    if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorNonexistentProperty);
    }

    const Accessor* pAccessor = _pModel->getSafe<Accessor>(
        &_pModel->accessors,
        primitive.attributes.at("POSITION"));
    if (!pAccessor) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorNonexistentProperty);
    }

    return PropertyAttributePropertyView<T, Normalized>(
        classProperty,
        pAccessor->count);
  }

  template <typename T, bool Normalized>
  PropertyAttributePropertyView<T, Normalized> getPropertyViewImpl(
      const MeshPrimitive& primitive,
      const std::string& propertyId,
      const ClassProperty& classProperty) const {
    auto propertyAttributePropertyIter =
        _pPropertyAttribute->properties.find(propertyId);
    if (propertyAttributePropertyIter ==
        _pPropertyAttribute->properties.end()) {
      if (!classProperty.required && classProperty.defaultProperty) {
        // If the property was omitted from the property attribute, it is still
        // technically valid if it specifies a default value. Try to create a
        // view that just returns the default value.
        return getEmptyPropertyViewWithDefault<T, Normalized>(
            primitive,
            classProperty);
      }

      // Otherwise, the property is erroneously nonexistent.
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorNonexistentProperty);
    }

    const PropertyAttributeProperty& propertyAttributeProperty =
        propertyAttributePropertyIter->second;

    return createPropertyView<T, Normalized>(
        primitive,
        classProperty,
        propertyAttributeProperty);
  }

  template <typename Callback, bool Normalized>
  void getScalarPropertyViewImpl(
      const MeshPrimitive& primitive,
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyId,
          getPropertyViewImpl<int8_t, Normalized>(
              primitive,
              propertyId,
              classProperty));
      return;
    case PropertyComponentType::Uint8:
      callback(
          propertyId,
          getPropertyViewImpl<uint8_t, Normalized>(
              primitive,
              propertyId,
              classProperty));
      return;
    case PropertyComponentType::Int16:
      callback(
          propertyId,
          getPropertyViewImpl<int16_t, Normalized>(
              primitive,
              propertyId,
              classProperty));
      return;
    case PropertyComponentType::Uint16:
      callback(
          propertyId,
          getPropertyViewImpl<uint16_t, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyId,
          getPropertyViewImpl<float, false>(
              primitive,
              propertyId,
              classProperty));
      break;
    default:
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty));
      break;
    }
  }

  template <typename Callback, glm::length_t N, bool Normalized>
  void getVecNPropertyViewImpl(
      const MeshPrimitive& primitive,
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyId,
          getPropertyViewImpl<glm::vec<N, int8_t>, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyId,
          getPropertyViewImpl<glm::vec<N, uint8_t>, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyId,
          getPropertyViewImpl<glm::vec<N, int16_t>, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyId,
          getPropertyViewImpl<glm::vec<N, uint16_t>, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyId,
          getPropertyViewImpl<glm::vec<N, float>, false>(
              primitive,
              propertyId,
              classProperty));
      break;
    default:
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty));
      break;
    }
  }

  template <typename Callback, bool Normalized>
  void getVecNPropertyViewImpl(
      const MeshPrimitive& primitive,
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    const glm::length_t N = getDimensionsFromPropertyType(type);
    switch (N) {
    case 2:
      getVecNPropertyViewImpl<Callback, 2, Normalized>(
          primitive,
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 3:
      getVecNPropertyViewImpl<Callback, 3, Normalized>(
          primitive,
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 4:
      getVecNPropertyViewImpl<Callback, 4, Normalized>(
          primitive,
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    default:
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::ErrorTypeMismatch));
      break;
    }
  }

  template <typename Callback, glm::length_t N, bool Normalized>
  void getMatNPropertyViewImpl(
      const MeshPrimitive& primitive,
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyId,
          getPropertyViewImpl<glm::mat<N, N, int8_t>, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyId,
          getPropertyViewImpl<glm::mat<N, N, uint8_t>, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyId,
          getPropertyViewImpl<glm::mat<N, N, int16_t>, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyId,
          getPropertyViewImpl<glm::mat<N, N, uint16_t>, Normalized>(
              primitive,
              propertyId,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyId,
          getPropertyViewImpl<glm::mat<N, N, float>, false>(
              primitive,
              propertyId,
              classProperty));
      break;
    default:
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty));
      break;
    }
  }

  template <typename Callback, bool Normalized>
  void getMatNPropertyViewImpl(
      const MeshPrimitive& primitive,
      const std::string& propertyId,
      const ClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    glm::length_t N = getDimensionsFromPropertyType(type);
    switch (N) {
    case 2:
      getMatNPropertyViewImpl<Callback, 2, Normalized>(
          primitive,
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 3:
      getMatNPropertyViewImpl<Callback, 3, Normalized>(
          primitive,
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    case 4:
      getMatNPropertyViewImpl<Callback, 4, Normalized>(
          primitive,
          propertyId,
          classProperty,
          componentType,
          std::forward<Callback>(callback));
      break;
    default:
      callback(
          propertyId,
          PropertyAttributePropertyView<uint8_t>(
              PropertyAttributePropertyViewStatus::ErrorTypeMismatch));
      break;
    }
  }

  template <typename T, bool Normalized>
  PropertyAttributePropertyView<T, Normalized> createPropertyView(
      const MeshPrimitive& primitive,
      const ClassProperty& classProperty,
      const PropertyAttributeProperty& propertyAttributeProperty) const {
    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    if (classProperty.normalized != Normalized) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
    }

    if (primitive.attributes.find(propertyAttributeProperty.attribute) ==
        primitive.attributes.end()) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorMissingAttribute);
    }

    const Accessor* pAccessor = _pModel->getSafe<Accessor>(
        &_pModel->accessors,
        primitive.attributes.at(propertyAttributeProperty.attribute));
    if (!pAccessor) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorInvalidAccessor);
    }

    if (getAccessorTypeAsPropertyType(*pAccessor) != type) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::ErrorAccessorTypeMismatch);
    }

    if (getAccessorComponentTypeAsPropertyComponentType(*pAccessor) !=
        componentType) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::
              ErrorAccessorComponentTypeMismatch);
    }

    if (pAccessor->normalized != Normalized) {
      return PropertyAttributePropertyView<T, Normalized>(
          PropertyAttributePropertyViewStatus::
              ErrorAccessorNormalizationMismatch);
    }

    AccessorView<T> accessorView = AccessorView<T>(*_pModel, *pAccessor);
    if (accessorView.status() != AccessorViewStatus::Valid) {
      switch (accessorView.status()) {
      case AccessorViewStatus::InvalidBufferViewIndex:
        return PropertyAttributePropertyView<T, Normalized>(
            PropertyAttributePropertyViewStatus::ErrorInvalidBufferView);
      case AccessorViewStatus::InvalidBufferIndex:
        return PropertyAttributePropertyView<T, Normalized>(
            PropertyAttributePropertyViewStatus::ErrorInvalidBuffer);
      case AccessorViewStatus::BufferViewTooSmall:
        return PropertyAttributePropertyView<T, Normalized>(
            PropertyAttributePropertyViewStatus::ErrorAccessorOutOfBounds);
      case AccessorViewStatus::BufferTooSmall:
        return PropertyAttributePropertyView<T, Normalized>(
            PropertyAttributePropertyViewStatus::ErrorBufferViewOutOfBounds);
      default:
        return PropertyAttributePropertyView<T, Normalized>(
            PropertyAttributePropertyViewStatus::ErrorInvalidAccessor);
      }
    }

    return PropertyAttributePropertyView<T, Normalized>(
        propertyAttributeProperty,
        classProperty,
        accessorView);
  }

  const Model* _pModel;
  const PropertyAttribute* _pPropertyAttribute;
  const Class* _pClass;

  PropertyAttributeViewStatus _status;
};

} // namespace CesiumGltf
