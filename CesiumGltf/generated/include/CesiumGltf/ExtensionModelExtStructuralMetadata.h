// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include <CesiumGltf/Library.h>
#include <CesiumGltf/PropertyAttribute.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTexture.h>
#include <CesiumGltf/Schema.h>
#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <optional>
#include <string>
#include <vector>

namespace CesiumGltf {
/**
 * @brief glTF extension that provides structural metadata about vertices,
 * texels, and features in a glTF asset.
 */
struct CESIUMGLTF_API ExtensionModelExtStructuralMetadata final
    : public CesiumUtility::ExtensibleObject {
  /**
   * @brief The original name of this type.
   */
  static constexpr const char* TypeName = "ExtensionModelExtStructuralMetadata";
  /** @brief The official name of the extension. This should be the same as its
   * key in the `extensions` object. */
  static constexpr const char* ExtensionName = "EXT_structural_metadata";

  /**
   * @brief An object defining classes and enums.
   */
  CesiumUtility::IntrusivePointer<CesiumGltf::Schema> schema;

  /**
   * @brief The URI (or IRI) of the external schema file.
   */
  std::optional<std::string> schemaUri;

  /**
   * @brief An array of property table definitions, which may be referenced by
   * index.
   */
  std::vector<CesiumGltf::PropertyTable> propertyTables;

  /**
   * @brief An array of property texture definitions, which may be referenced by
   * index.
   */
  std::vector<CesiumGltf::PropertyTexture> propertyTextures;

  /**
   * @brief An array of property attribute definitions, which may be referenced
   * by index.
   */
  std::vector<CesiumGltf::PropertyAttribute> propertyAttributes;

  /**
   * @brief Calculates the size in bytes of this object, including the contents
   * of all collections, pointers, and strings. This will NOT include the size
   * of any extensions attached to the object. Calling this method may be slow
   * as it requires traversing the object's entire structure.
   */
  int64_t getSizeBytes() const {
    int64_t accum = 0;
    accum += int64_t(sizeof(ExtensionModelExtStructuralMetadata));
    accum += CesiumUtility::ExtensibleObject::getSizeBytes() -
             int64_t(sizeof(CesiumUtility::ExtensibleObject));
    accum += this->schema->getSizeBytes();
    if (this->schemaUri) {
      accum += int64_t(this->schemaUri->capacity() * sizeof(char));
    }
    accum += int64_t(
        sizeof(CesiumGltf::PropertyTable) * this->propertyTables.capacity());
    for (const CesiumGltf::PropertyTable& value : this->propertyTables) {
      accum +=
          value.getSizeBytes() - int64_t(sizeof(CesiumGltf::PropertyTable));
    }
    accum += int64_t(
        sizeof(CesiumGltf::PropertyTexture) *
        this->propertyTextures.capacity());
    for (const CesiumGltf::PropertyTexture& value : this->propertyTextures) {
      accum +=
          value.getSizeBytes() - int64_t(sizeof(CesiumGltf::PropertyTexture));
    }
    accum += int64_t(
        sizeof(CesiumGltf::PropertyAttribute) *
        this->propertyAttributes.capacity());
    for (const CesiumGltf::PropertyAttribute& value :
         this->propertyAttributes) {
      accum +=
          value.getSizeBytes() - int64_t(sizeof(CesiumGltf::PropertyAttribute));
    }
    return accum;
  }
};
} // namespace CesiumGltf
