#pragma once

#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/PropertyTablePropertyView.h"
#include "CesiumGltf/PropertyType.h"

#include <glm/common.hpp>

#include <optional>

namespace CesiumGltf {
/**
 * @brief Indicates the status of a property table view.
 *
 * The {@link PropertyTableView} constructor always completes successfully.
 * However, it may not always reflect the actual content of the
 * {@link ExtensionExtStructuralMetadataPropertyTable}, but instead indicate that its
 * {@link PropertyTableView::size} is 0. This enumeration provides the reason.
 */
enum class PropertyTableViewStatus {
  /**
   * @brief This property table view is valid and ready to use.
   */
  Valid,

  /**
   * @brief The property table view's model does not contain an
   * EXT_structural_metadata extension.
   */
  ErrorMissingMetadataExtension,

  /**
   * @brief The property table view's model does not have a schema in its
   * EXT_structural_metadata extension.
   */
  ErrorMissingSchema,

  /**
   * @brief The property table's specified class could not be found in the
   * extension.
   */
  ErrorClassNotFound
};

/**
 * @brief Utility to retrieve the data of
 * {@link ExtensionExtStructuralMetadataPropertyTable}.
 *
 * This should be used to get a {@link PropertyTablePropertyView} of a property in the property table.
 * It will validate the EXT_structural_metadata format and ensure {@link PropertyTablePropertyView}
 * does not access out of bounds.
 */
class PropertyTableView {
public:
  /**
   * @brief Creates an instance of PropertyTableView.
   * @param model The Gltf Model that contains property table data.
   * @param propertyTable The {@link ExtensionExtStructuralMetadataPropertyTable}
   * from which the view will retrieve data.
   */
  PropertyTableView(
      const Model& model,
      const ExtensionExtStructuralMetadataPropertyTable& propertyTable);

  /**
   * @brief Gets the status of this property table view.
   *
   * Indicates whether the view accurately reflects the property table's data,
   * or whether an error occurred.
   *
   * @return The status of this property table view.
   */
  PropertyTableViewStatus status() const noexcept { return _status; }

  /**
   * @brief Get the number of elements in this PropertyTableView. If the
   * view is valid, this returns
   * {@link ExtensionExtStructuralMetadataPropertyTable::count}. Otherwise, this returns 0.
   *
   * @return The number of elements in this PropertyTableView.
   */
  int64_t size() const noexcept {
    return _status == PropertyTableViewStatus::Valid ? _pPropertyTable->count
                                                     : 0;
  }

  /**
   * @brief Finds the {@link ExtensionExtStructuralMetadataClassProperty} that
   * describes the type information of the property with the specified name.
   * @param propertyName The name of the property to retrieve the class for.
   * @return A pointer to the {@link ExtensionExtStructuralMetadataClassProperty}.
   * Return nullptr if the PropertyTableView is invalid or if no class
   * property was found.
   */
  const ExtensionExtStructuralMetadataClassProperty*
  getClassProperty(const std::string& propertyName) const;

  /**
   * @brief Gets a {@link PropertyTablePropertyView} that views the data of a property stored
   * in the {@link ExtensionExtStructuralMetadataPropertyTable}.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyTablePropertyView} retrieves the correct data. T must be one of the
   * following: a scalar (uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t,
   * uint64_t, int64_t, float, double), a glm vecN composed of one of the scalar
   * types, a glm matN composed of one of the scalar types, bool,
   * std::string_view, or {@link PropertyArrayView<T>} with T as one of the
   * aforementioned types.
   *
   * @param propertyName The name of the property to retrieve data from
   * @return A {@link PropertyTablePropertyView} of the property. If no valid property is
   * found, the property view will be invalid.
   */
  template <typename T>
  PropertyTablePropertyView<T>
  getPropertyView(const std::string& propertyName) const {
    if (this->size() <= 0) {
      return createInvalidPropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable);
    }

    const ExtensionExtStructuralMetadataClassProperty* pClassProperty =
        getClassProperty(propertyName);
    if (!pClassProperty) {
      return createInvalidPropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorPropertyDoesNotExist);
    }

    return getPropertyViewImpl<T>(propertyName, *pClassProperty);
  }

  /**
   * @brief Gets a {@link PropertyTablePropertyView} through a callback that accepts a
   * property name and a {@link PropertyTablePropertyView<T>} that views the data
   * of the property with the specified name.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyTablePropertyView} retrieves the correct data. T must be one of the
   * following: a scalar (uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t,
   * uint64_t, int64_t, float, double), a glm vecN composed of one of the scalar
   * types, a glm matN composed of one of the scalar types, bool,
   * std::string_view, or {@link PropertyArrayView<T>} with T as one of the
   * aforementioned types. If the property is invalid, an empty
   * {@link PropertyTablePropertyView} with an error status will be passed to the
   * callback. Otherwise, a valid property view will be passed to the callback.
   *
   * @param propertyName The name of the property to retrieve data from
   * @tparam callback A callback function that accepts a property name and a
   * {@link PropertyTablePropertyView<T>}
   */
  template <typename Callback>
  void
  getPropertyView(const std::string& propertyName, Callback&& callback) const {
    if (this->size() <= 0) {
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable));
      return;
    }

    const ExtensionExtStructuralMetadataClassProperty* pClassProperty =
        getClassProperty(propertyName);
    if (!pClassProperty) {
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorPropertyDoesNotExist));
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
    } else if (type == PropertyType::Scalar) {
      getScalarPropertyViewImpl(
          propertyName,
          *pClassProperty,
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
    } else if (type == PropertyType::String) {
      callback(
          propertyName,
          getPropertyViewImpl<std::string_view>(propertyName, *pClassProperty));
    } else if (type == PropertyType::Boolean) {
      callback(
          propertyName,
          getPropertyViewImpl<bool>(propertyName, *pClassProperty));
    } else {
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
    }
  }

  /**
   * @brief Iterates over each property in the
   * {@link ExtensionExtStructuralMetadataPropertyTable} with a callback that accepts a
   * property name and a {@link PropertyTablePropertyView<T>} to view the data
   * stored in the {@link ExtensionExtStructuralMetadataPropertyTableProperty}.
   *
   * This method will validate the EXT_structural_metadata format to ensure
   * {@link PropertyTablePropertyView} retrieves the correct data. T must be one of the
   * following: a scalar (uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t,
   * uint64_t, int64_t, float, double), a glm vecN composed of one of the scalar
   * types, a glm matN composed of one of the scalar types, bool,
   * std::string_view, or {@link PropertyArrayView<T>} with T as one of the
   * aforementioned types. If the property is invalid, an empty
   * {@link PropertyTablePropertyView} with an error status code will be passed to the
   * callback. Otherwise, a valid property view will be passed to
   * the callback.
   *
   * @param propertyName The name of the property to retrieve data from
   * @tparam callback A callback function that accepts property name and
   * {@link PropertyTablePropertyView<T>}
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
          getPropertyViewImpl<PropertyArrayView<int8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<uint8_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<int16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<uint16_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<int32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<uint32_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<int64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<uint64_t>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<float>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<double>>(
              propertyName,
              classProperty));
      break;
    default:
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
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
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, int8_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, uint8_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, int16_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, uint16_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, int32_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, uint32_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, int64_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, uint64_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, float>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::vec<N, double>>>(
              propertyName,
              classProperty));
      break;
    default:
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
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
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
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
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, int8_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, uint8_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, int16_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint16:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, uint16_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, int32_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, uint32_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Int64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, int64_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Uint64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, uint64_t>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float32:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, float>>>(
              propertyName,
              classProperty));
      break;
    case PropertyComponentType::Float64:
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<glm::mat<N, N, double>>>(
              propertyName,
              classProperty));
      break;
    default:
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
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
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
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
          getPropertyViewImpl<PropertyArrayView<bool>>(
              propertyName,
              classProperty));

    } else if (type == PropertyType::String) {
      callback(
          propertyName,
          getPropertyViewImpl<PropertyArrayView<std::string_view>>(
              propertyName,
              classProperty));
    } else {
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
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
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
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
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
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
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
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
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
      break;
    }
  }

  template <typename Callback>
  void getScalarPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      PropertyComponentType componentType,
      Callback&& callback) const {
    switch (componentType) {
    case PropertyComponentType::Int8:
      callback(
          propertyName,
          getPropertyViewImpl<int8_t>(propertyName, classProperty));
      return;
    case PropertyComponentType::Uint8:
      callback(
          propertyName,
          getPropertyViewImpl<uint8_t>(propertyName, classProperty));
      return;
    case PropertyComponentType::Int16:
      callback(
          propertyName,
          getPropertyViewImpl<int16_t>(propertyName, classProperty));
      return;
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
      callback(
          propertyName,
          createInvalidPropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
      break;
    }
  }

  template <typename T>
  PropertyTablePropertyView<T> getPropertyViewImpl(
      const std::string& propertyName,
      const ExtensionExtStructuralMetadataClassProperty& classProperty) const {
    auto propertyTablePropertyIter =
        _pPropertyTable->properties.find(propertyName);
    if (propertyTablePropertyIter == _pPropertyTable->properties.end()) {
      return createInvalidPropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorPropertyDoesNotExist);
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
  PropertyTablePropertyView<T> getNumericOrBooleanPropertyValues(
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      const ExtensionExtStructuralMetadataPropertyTableProperty&
          propertyTableProperty) const {
    if (classProperty.array) {
      return createInvalidPropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return createInvalidPropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorTypeMismatch);
    }
    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return createInvalidPropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    gsl::span<const std::byte> values;
    const auto status = getBufferSafe(propertyTableProperty.values, values);
    if (status != PropertyTablePropertyViewStatus::Valid) {
      return createInvalidPropertyView<T>(status);
    }

    if (values.size() % sizeof(T) != 0) {
      return createInvalidPropertyView<T>(
          PropertyTablePropertyViewStatus::
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
          PropertyTablePropertyViewStatus::
              ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
    }

    return PropertyTablePropertyView<T>(
        PropertyTablePropertyViewStatus::Valid,
        values,
        _pPropertyTable->count,
        classProperty.normalized);
  }

  PropertyTablePropertyView<std::string_view> getStringPropertyValues(
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      const ExtensionExtStructuralMetadataPropertyTableProperty&
          propertyTableProperty) const;

  template <typename T>
  PropertyTablePropertyView<PropertyArrayView<T>>
  getPrimitiveArrayPropertyValues(
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      const ExtensionExtStructuralMetadataPropertyTableProperty&
          propertyTableProperty) const {
    if (!classProperty.array) {
      return createInvalidPropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return createInvalidPropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return createInvalidPropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    gsl::span<const std::byte> values;
    auto status = getBufferSafe(propertyTableProperty.values, values);
    if (status != PropertyTablePropertyViewStatus::Valid) {
      return createInvalidPropertyView<PropertyArrayView<T>>(status);
    }

    if (values.size() % sizeof(T) != 0) {
      return createInvalidPropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::
              ErrorBufferViewSizeNotDivisibleByTypeSize);
    }

    const int64_t fixedLengthArrayCount = classProperty.count.value_or(0);
    if (fixedLengthArrayCount > 0 && propertyTableProperty.arrayOffsets >= 0) {
      return createInvalidPropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::
              ErrorArrayCountAndOffsetBufferCoexist);
    }

    if (fixedLengthArrayCount <= 0 && propertyTableProperty.arrayOffsets < 0) {
      return createInvalidPropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::
              ErrorArrayCountAndOffsetBufferDontExist);
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
        return createInvalidPropertyView<PropertyArrayView<T>>(
            PropertyTablePropertyViewStatus::
                ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
      }

      return PropertyTablePropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::Valid,
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
      return createInvalidPropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);
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
    if (status != PropertyTablePropertyViewStatus::Valid) {
      return createInvalidPropertyView<PropertyArrayView<T>>(status);
    }

    return PropertyTablePropertyView<PropertyArrayView<T>>(
        PropertyTablePropertyViewStatus::Valid,
        values,
        arrayOffsets,
        gsl::span<const std::byte>(),
        arrayOffsetType,
        PropertyComponentType::None,
        0,
        static_cast<size_t>(_pPropertyTable->count),
        classProperty.normalized);
  }

  PropertyTablePropertyView<PropertyArrayView<std::string_view>>
  getStringArrayPropertyValues(
      const ExtensionExtStructuralMetadataClassProperty& classProperty,
      const ExtensionExtStructuralMetadataPropertyTableProperty&
          propertyTableProperty) const;

  PropertyTablePropertyViewStatus getBufferSafe(
      int32_t bufferView,
      gsl::span<const std::byte>& buffer) const noexcept;

  PropertyTablePropertyViewStatus getArrayOffsetsBufferSafe(
      int32_t arrayOffsetsBufferView,
      PropertyComponentType arrayOffsetType,
      size_t valuesBufferSize,
      size_t propertyTableCount,
      bool checkBitsSize,
      gsl::span<const std::byte>& arrayOffsetsBuffer) const noexcept;

  PropertyTablePropertyViewStatus getStringOffsetsBufferSafe(
      int32_t stringOffsetsBufferView,
      PropertyComponentType stringOffsetType,
      size_t valuesBufferSize,
      size_t propertyTableCount,
      gsl::span<const std::byte>& stringOffsetsBuffer) const noexcept;

  template <typename T>
  static PropertyTablePropertyView<T> createInvalidPropertyView(
      PropertyTablePropertyViewStatus invalidStatus) noexcept {
    return PropertyTablePropertyView<T>(
        invalidStatus,
        gsl::span<const std::byte>(),
        0,
        false);
  }

  const Model* _pModel;
  const ExtensionExtStructuralMetadataPropertyTable* _pPropertyTable;
  const ExtensionExtStructuralMetadataClass* _pClass;
  PropertyTableViewStatus _status;
};
} // namespace CesiumGltf
