#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyAttribute.h>
#include <CesiumGltf/PropertyAttributeView.h>
#include <CesiumGltf/PropertyType.h>

#include <string>

namespace CesiumGltf {
PropertyType getAccessorTypeAsPropertyType(const Accessor& accessor) {
  if (accessor.type == Accessor::Type::SCALAR) {
    return PropertyType::Scalar;
  }
  if (accessor.type == Accessor::Type::VEC2) {
    return PropertyType::Vec2;
  }
  if (accessor.type == Accessor::Type::VEC3) {
    return PropertyType::Vec3;
  }
  if (accessor.type == Accessor::Type::VEC4) {
    return PropertyType::Vec4;
  }
  if (accessor.type == Accessor::Type::MAT2) {
    return PropertyType::Mat2;
  }
  if (accessor.type == Accessor::Type::MAT3) {
    return PropertyType::Mat3;
  }
  if (accessor.type == Accessor::Type::MAT4) {
    return PropertyType::Mat4;
  }

  return PropertyType::Invalid;
}

PropertyComponentType
getAccessorComponentTypeAsPropertyComponentType(const Accessor& accessor) {
  switch (accessor.componentType) {
  case Accessor::ComponentType::BYTE:
    return PropertyComponentType::Int8;
  case Accessor::ComponentType::UNSIGNED_BYTE:
    return PropertyComponentType::Uint8;
  case Accessor::ComponentType::SHORT:
    return PropertyComponentType::Int16;
  case Accessor::ComponentType::UNSIGNED_SHORT:
    return PropertyComponentType::Uint16;
  case Accessor::ComponentType::FLOAT:
    return PropertyComponentType::Float32;
  default:
    return PropertyComponentType::None;
  }
}

PropertyAttributeView::PropertyAttributeView(
    const Model& model,
    const PropertyAttribute& propertyAttribute) noexcept
    : _pModel(&model),
      _pPropertyAttribute(&propertyAttribute),
      _pClass(nullptr),
      _status() {
  const ExtensionModelExtStructuralMetadata* pMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();

  if (!pMetadata) {
    this->_status = PropertyAttributeViewStatus::ErrorMissingMetadataExtension;
    return;
  }

  if (!pMetadata->schema) {
    this->_status = PropertyAttributeViewStatus::ErrorMissingSchema;
    return;
  }

  const auto& classIt =
      pMetadata->schema->classes.find(propertyAttribute.classProperty);
  if (classIt == pMetadata->schema->classes.end()) {
    this->_status = PropertyAttributeViewStatus::ErrorClassNotFound;
    return;
  }

  this->_pClass = &classIt->second;
}

const ClassProperty*
PropertyAttributeView::getClassProperty(const std::string& propertyId) const {
  if (_status != PropertyAttributeViewStatus::Valid) {
    return nullptr;
  }

  auto propertyIter = _pClass->properties.find(propertyId);
  if (propertyIter == _pClass->properties.end()) {
    return nullptr;
  }

  return &propertyIter->second;
}

} // namespace CesiumGltf
