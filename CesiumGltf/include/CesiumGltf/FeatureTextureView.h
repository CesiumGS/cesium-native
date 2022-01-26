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

enum class FeatureTextureViewStatus {
  Valid,
  InvalidUninitialized,
  InvalidMissingMetadataExtension,
  InvalidMissingSchema,
  InvalidClassNotFound,
  InvalidClassPropertyNotFound,
  InvalidPropertyViewStatus
};

class FeatureTextureView {
public:
  FeatureTextureView() noexcept;

  FeatureTextureView(
      const Model& model,
      const FeatureTexture& featureTexture) noexcept;

  FeatureTextureViewStatus status() const noexcept { return this->_status; }

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