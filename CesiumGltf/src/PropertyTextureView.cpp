#include "CesiumGltf/PropertyTextureView.h"

namespace CesiumGltf {
PropertyTextureView::PropertyTextureView() noexcept
    : _pModel(nullptr),
      _pPropertyTexture(nullptr),
      _pClass(nullptr),
      _propertyViews(),
      _status(PropertyTextureViewStatus::ErrorUninitialized) {}

PropertyTextureView::PropertyTextureView(
    const Model& model,
    const PropertyTexture& propertyTexture) noexcept
    : _pModel(&model),
      _pPropertyTexture(&propertyTexture),
      _pClass(nullptr),
      _propertyViews(),
      _status(PropertyTextureViewStatus::ErrorUninitialized) {

  const ExtensionModelExtStructuralMetadata* pMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();

  if (!pMetadata) {
    this->_status = PropertyTextureViewStatus::ErrorMissingMetadataExtension;
    return;
  }

  if (!pMetadata->schema) {
    this->_status = PropertyTextureViewStatus::ErrorMissingSchema;
    return;
  }

  const auto& classIt =
      pMetadata->schema->classes.find(propertyTexture.classProperty);
  if (classIt == pMetadata->schema->classes.end()) {
    this->_status = PropertyTextureViewStatus::ErrorClassNotFound;
    return;
  }

  this->_pClass = &classIt->second;

  this->_propertyViews.reserve(propertyTexture.properties.size());
  for (const auto& property : propertyTexture.properties) {
    auto classPropertyIt = this->_pClass->properties.find(property.first);

    if (classPropertyIt == this->_pClass->properties.end()) {
      this->_status = PropertyTextureViewStatus::ErrorClassPropertyNotFound;
      return;
    }

    this->_propertyViews[property.first] = PropertyTexturePropertyView(
        model,
        classPropertyIt->second,
        property.second);
  }

  this->_status = PropertyTextureViewStatus::Valid;
}

const ClassProperty*
PropertyTextureView::getClassProperty(const std::string& propertyName) const {
  if (_status != PropertyTextureViewStatus::Valid) {
    return nullptr;
  }

  auto propertyIter = _pClass->properties.find(propertyName);
  if (propertyIter == _pClass->properties.end()) {
    return nullptr;
  }

  return &propertyIter->second;
}
} // namespace CesiumGltf
