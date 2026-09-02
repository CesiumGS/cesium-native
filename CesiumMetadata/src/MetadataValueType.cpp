#include <Cesium3DTiles/ClassProperty.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumMetadata/MetadataValueType.h>

namespace CesiumMetadata {
MetadataValueType::MetadataValueType()
    : type(PropertyType::Invalid),
      componentType(PropertyComponentType::None),
      array(false) {}

MetadataValueType::MetadataValueType(
    PropertyType type_,
    PropertyComponentType componentType_,
    bool array_)
    : type(type_), componentType(componentType_), array(array_) {}

MetadataValueType::MetadataValueType(
    const Cesium3DTiles::ClassProperty& property)
    : type(convertStringToPropertyType(property.type)),
      componentType(
          property.componentType
              ? convertStringToPropertyComponentType(*property.componentType)
              : PropertyComponentType::None),
      array(property.array) {}

MetadataValueType::MetadataValueType(const CesiumGltf::ClassProperty& property)
    : type(convertStringToPropertyType(property.type)),
      componentType(
          property.componentType
              ? convertStringToPropertyComponentType(*property.componentType)
              : PropertyComponentType::None),
      array(property.array) {}

} // namespace CesiumMetadata
