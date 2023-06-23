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
 * However, it may not always reflect the actual content of the {@link PropertyTable},
 * but instead indicate that its {@link PropertyTableView::size} is 0.
 * This enumeration provides the reason.
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
 * @brief Utility to retrieve the data of {@link PropertyTable}.
 *
 * This should be used to get a {@link PropertyTablePropertyView} of a property in the property table.
 * It will validate the EXT_structural_metadata format and ensure {@link PropertyTablePropertyView}
 * does not access out of bounds.
 */
class PropertyTableView {
public:
  /**
   * @brief Creates an instance of PropertyTableView.
   * @param model The glTF Model that contains the property table data.
   * @param propertyTable The {@link PropertyTable}
   * from which the view will retrieve data.
   */
  PropertyTableView(const Model& model, const PropertyTable& propertyTable);

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
   * view is valid, this returns {@link PropertyTable::count}. Otherwise, this returns 0.
   *
   * @return The number of elements in this PropertyTableView.
   */
  int64_t size() const noexcept {
    return _status == PropertyTableViewStatus::Valid ? _pPropertyTable->count
                                                     : 0;
  }

  /**
   * @brief Finds the {@link ClassProperty} that
   * describes the type information of the property with the specified name.
   * @param propertyName The name of the property to retrieve the class for.
   * @return A pointer to the {@link ClassProperty}.
   * Return nullptr if the PropertyTableView is invalid or if no class
   * property was found.
   */
  const ClassProperty* getClassProperty(const std::string& propertyName) const;

  /**
   * @brief Gets a {@link PropertyTablePropertyView} that views the data of a property stored
   * in the {@link PropertyTable}.
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
      return PropertyTablePropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable);
    }

    const ClassProperty* pClassProperty = getClassProperty(propertyName);
    if (!pClassProperty) {
      return PropertyTablePropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorNonexistentProperty);
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable));
      return;
    }

    const ClassProperty* pClassProperty = getClassProperty(propertyName);
    if (!pClassProperty) {
      callback(
          propertyName,
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorNonexistentProperty));
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
    }
  }

  /**
   * @brief Iterates over each property in the {@link PropertyTable} with a callback
   * that accepts a property name and a {@link PropertyTablePropertyView<T>} to view
   * the data stored in the {@link PropertyTableProperty}.
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
  template <typename Callback>
  void getScalarArrayPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
      break;
    }
  }

  template <typename Callback, glm::length_t N>
  void getVecNArrayPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
      break;
    }
  }

  template <typename Callback>
  void getVecNArrayPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    glm::length_t N = getDimensionsFromPropertyType(type);
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
      break;
    }
  }

  template <typename Callback, glm::length_t N>
  void getMatNArrayPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
      break;
    }
  }

  template <typename Callback>
  void getMatNArrayPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    const glm::length_t N = getDimensionsFromPropertyType(type);
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
      break;
    }
  }

  template <typename Callback>
  void getArrayPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
    }
  }

  template <typename Callback, glm::length_t N>
  void getVecNPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
      break;
    }
  }

  template <typename Callback>
  void getVecNPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    const glm::length_t N = getDimensionsFromPropertyType(type);
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
      break;
    }
  }

  template <typename Callback, glm::length_t N>
  void getMatNPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
      break;
    }
  }

  template <typename Callback>
  void getMatNPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
      PropertyType type,
      PropertyComponentType componentType,
      Callback&& callback) const {
    glm::length_t N = getDimensionsFromPropertyType(type);
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorTypeMismatch));
      break;
    }
  }

  template <typename Callback>
  void getScalarPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty,
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
          PropertyTablePropertyView<uint8_t>(
              PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch));
      break;
    }
  }

  template <typename T>
  PropertyTablePropertyView<T> getPropertyViewImpl(
      const std::string& propertyName,
      const ClassProperty& classProperty) const {
    auto propertyTablePropertyIter =
        _pPropertyTable->properties.find(propertyName);
    if (propertyTablePropertyIter == _pPropertyTable->properties.end()) {
      return PropertyTablePropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorNonexistentProperty);
    }

    const PropertyTableProperty& propertyTableProperty =
        propertyTablePropertyIter->second;

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
      const ClassProperty& classProperty,
      const PropertyTableProperty& propertyTableProperty) const {
    if (classProperty.array) {
      return PropertyTablePropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyTablePropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorTypeMismatch);
    }
    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyTablePropertyView<T>(
          PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    gsl::span<const std::byte> values;
    const auto status = getBufferSafe(propertyTableProperty.values, values);
    if (status != PropertyTablePropertyViewStatus::Valid) {
      return PropertyTablePropertyView<T>(status);
    }

    if (values.size() % sizeof(T) != 0) {
      return PropertyTablePropertyView<T>(
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
      return PropertyTablePropertyView<T>(
          PropertyTablePropertyViewStatus::
              ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
    }

    return PropertyTablePropertyView<T>(
        values,
        _pPropertyTable->count,
        classProperty.normalized);
  }

  PropertyTablePropertyView<std::string_view> getStringPropertyValues(
      const ClassProperty& classProperty,
      const PropertyTableProperty& propertyTableProperty) const;

  template <typename T>
  PropertyTablePropertyView<PropertyArrayView<T>>
  getPrimitiveArrayPropertyValues(
      const ClassProperty& classProperty,
      const PropertyTableProperty& propertyTableProperty) const {
    if (!classProperty.array) {
      return PropertyTablePropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
    }

    const PropertyType type = convertStringToPropertyType(classProperty.type);
    if (TypeToPropertyType<T>::value != type) {
      return PropertyTablePropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::ErrorTypeMismatch);
    }

    const PropertyComponentType componentType =
        convertStringToPropertyComponentType(
            classProperty.componentType.value_or(""));
    if (TypeToPropertyType<T>::component != componentType) {
      return PropertyTablePropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
    }

    gsl::span<const std::byte> values;
    auto status = getBufferSafe(propertyTableProperty.values, values);
    if (status != PropertyTablePropertyViewStatus::Valid) {
      return PropertyTablePropertyView<PropertyArrayView<T>>(status);
    }

    if (values.size() % sizeof(T) != 0) {
      return PropertyTablePropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::
              ErrorBufferViewSizeNotDivisibleByTypeSize);
    }

    const int64_t fixedLengthArrayCount = classProperty.count.value_or(0);
    if (fixedLengthArrayCount > 0 && propertyTableProperty.arrayOffsets >= 0) {
      return PropertyTablePropertyView<PropertyArrayView<T>>(
          PropertyTablePropertyViewStatus::
              ErrorArrayCountAndOffsetBufferCoexist);
    }

    if (fixedLengthArrayCount <= 0 && propertyTableProperty.arrayOffsets < 0) {
      return PropertyTablePropertyView<PropertyArrayView<T>>(
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
        return PropertyTablePropertyView<PropertyArrayView<T>>(
            PropertyTablePropertyViewStatus::
                ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
      }

      return PropertyTablePropertyView<PropertyArrayView<T>>(
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
      return PropertyTablePropertyView<PropertyArrayView<T>>(
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
      return PropertyTablePropertyView<PropertyArrayView<T>>(status);
    }

    return PropertyTablePropertyView<PropertyArrayView<T>>(
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
      const ClassProperty& classProperty,
      const PropertyTableProperty& propertyTableProperty) const;

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

  const Model* _pModel;
  const PropertyTable* _pPropertyTable;
  const Class* _pClass;
  PropertyTableViewStatus _status;
};
} // namespace CesiumGltf
