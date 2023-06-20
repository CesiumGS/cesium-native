#pragma once

#include "CesiumGltf/Class.h"
#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/PropertyTexture.h"
#include "CesiumGltf/PropertyTexturePropertyView.h"
#include "CesiumGltf/Texture.h"
#include "Image.h"
#include "ImageCesium.h"
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
   * @brief This property texture view is not initialized.
   */
  ErrorUninitialized,

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
  ErrorClassNotFound,

  /**
   * @brief A property name specified in the property texture could not be found
   * in the class.
   */
  ErrorClassPropertyNotFound
};

/**
 * @brief A view on the {@link PropertyTexture}.
 *
 * Provides access to views on the property texture properties.
 */
class PropertyTextureView {
public:
  /**
   * @brief Construct an uninitialized, invalid property texture view.
   */
  PropertyTextureView() noexcept;

  /**
   * @brief Construct a view for the property texture.
   *
   * @param model The glTF in which to look for the property texture's data.
   * @param propertyTexture The property texture to create a view for.
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
   * @brief Get the views for this property texture's properties.
   */
  const std::unordered_map<std::string, PropertyTexturePropertyView>&
  getProperties() const noexcept {
    return this->_propertyViews;
  }

private:
  const Model* _pModel;
  const PropertyTexture* _pPropertyTexture;
  const Class* _pClass;
  std::unordered_map<std::string, PropertyTexturePropertyView> _propertyViews;
  PropertyTextureViewStatus _status;
};
} // namespace CesiumGltf
