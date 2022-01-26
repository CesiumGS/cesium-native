#pragma once

#include "CesiumGltf/Class.h"
#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/ExtensionModelExtFeatureMetadata.h"
#include "CesiumGltf/FeatureTexture.h"
#include "CesiumGltf/FeatureTexturePropertyView.h"
#include "CesiumGltf/Texture.h"
#include "CesiumGltf/TextureAccessor.h"
#include "Image.h"
#include "ImageCesium.h"
#include "Model.h"

namespace CesiumGltf {

/**
 * @brief Indicates the status of a feature texture view.
 *
 * The {@link FeatureTextureView} constructor always completes successfully.
 * However it may not always reflect the actual content of the
 * {@link FeatureTexture}. This enumeration provides the reason.
 */
enum class FeatureTextureViewStatus {
  /**
   * @brief This feature texture view is valid and ready to use.
   */
  Valid,

  /**
   * @brief This feature texture view is not initialized.
   */
  InvalidUninitialized,

  /**
   * @brief The glTF is missing the EXT_feature_metadata extension.
   */
  InvalidMissingMetadataExtension,

  /**
   * @brief The glTF EXT_feature_metadata extension doesn't contain a schema.
   */
  InvalidMissingSchema,

  /**
   * @brief The feature texture's specified class could not be found in the
   * extension.
   *
   */
  InvalidClassNotFound,

  /**
   * @brief A property name specified in the feature texture could not be found
   * in the class.
   */
  InvalidClassPropertyNotFound,

  /**
   * @brief A property view for one of this feature texture's properties failed
   * to initialize successfully. Look for the invalid property view's status to
   * find why it failed.
   */
  InvalidPropertyViewStatus
};

/**
 * @brief A view on the {@link FeatureTexture}.
 *
 * Provides access to views on the feature texture properties.
 */
class FeatureTextureView {
public:
  /**
   * @brief Construct an uninitialized, invalid feature texture view.
   */
  FeatureTextureView() noexcept;

  /**
   * @brief Construct a view for the feature texture.
   *
   * @param model The glTF in which to look for the feature texture's data.
   * @param featureTexture The feature texture to create a view for.
   */
  FeatureTextureView(
      const Model& model,
      const FeatureTexture& featureTexture) noexcept;

  /**
   * @brief Gets the status of this feature texture view.
   *
   * Indicates whether the view accurately reflects the feature texture's data,
   * or whether an error occurred.
   */
  FeatureTextureViewStatus status() const noexcept { return this->_status; }

  /**
   * @brief Get the views for this feature texture's properties.
   */
  const std::unordered_map<std::string, FeatureTexturePropertyView>&
  getProperties() const noexcept {
    return this->_propertyViews;
  }

private:
  const Model* _pModel;
  const FeatureTexture* _pFeatureTexture;
  const Class* _pClass;
  std::unordered_map<std::string, FeatureTexturePropertyView> _propertyViews;
  FeatureTextureViewStatus _status;
};
} // namespace CesiumGltf