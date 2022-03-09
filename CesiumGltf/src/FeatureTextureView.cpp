#include "CesiumGltf/FeatureTextureView.h"

namespace CesiumGltf {

FeatureTextureView::FeatureTextureView() noexcept
    : _pModel(nullptr),
      _pFeatureTexture(nullptr),
      _pClass(nullptr),
      _propertyViews(),
      _status(FeatureTextureViewStatus::InvalidUninitialized) {}

FeatureTextureView::FeatureTextureView(
    const Model& model,
    const FeatureTexture& featureTexture) noexcept
    : _pModel(&model),
      _pFeatureTexture(&featureTexture),
      _pClass(nullptr),
      _propertyViews(),
      _status(FeatureTextureViewStatus::InvalidUninitialized) {

  const ExtensionModelExtFeatureMetadata* pMetadata =
      model.getExtension<ExtensionModelExtFeatureMetadata>();

  if (!pMetadata) {
    this->_status = FeatureTextureViewStatus::InvalidMissingMetadataExtension;
    return;
  }

  if (!pMetadata->schema) {
    this->_status = FeatureTextureViewStatus::InvalidMissingSchema;
    return;
  }

  const auto& classIt =
      pMetadata->schema->classes.find(featureTexture.classProperty);
  if (classIt == pMetadata->schema->classes.end()) {
    this->_status = FeatureTextureViewStatus::InvalidClassNotFound;
    return;
  }

  this->_pClass = &classIt->second;

  this->_propertyViews.reserve(featureTexture.properties.size());
  for (const auto& property : featureTexture.properties) {
    auto classPropertyIt = this->_pClass->properties.find(property.first);

    if (classPropertyIt == this->_pClass->properties.end()) {
      this->_status = FeatureTextureViewStatus::InvalidClassPropertyNotFound;
      return;
    }

    this->_propertyViews[property.first] = FeatureTexturePropertyView(
        model,
        classPropertyIt->second,
        property.second);
  }

  for (const auto& propertyView : this->_propertyViews) {
    if (propertyView.second.status() !=
        FeatureTexturePropertyViewStatus::Valid) {
      this->_status = FeatureTextureViewStatus::InvalidPropertyViewStatus;
      return;
    }
  }

  this->_status = FeatureTextureViewStatus::Valid;
}
} // namespace CesiumGltf