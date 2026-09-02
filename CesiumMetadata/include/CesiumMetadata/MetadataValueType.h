#pragma once

#include <CesiumMetadata/PropertyType.h>
#include <CesiumMetadata/PropertyTypeTraits.h>

namespace Cesium3DTiles {
struct ClassProperty;
}

namespace CesiumGltf {
struct ClassProperty;
}

namespace CesiumMetadata {
/**
 * @brief Captures the type of a metadata property or value as defined by the
 * @ref CesiumGltf::ExtensionModelExtStructuralMetadata spec.
 */
struct MetadataValueType {
  /**
   * @brief The @ref PropertyType of the value.
   */
  PropertyType type = PropertyType::Invalid;
  /**
   * @brief The @ref PropertyComponentType of the value.
   */
  PropertyComponentType componentType = PropertyComponentType::None;
  /**
   * @brief Whether or not the value is an array.
   */
  bool array = false;

  /**
   * @brief Constructs a default, invalid value type.
   */
  MetadataValueType();

  /**
   * @brief Constructs a value type from the given parameters.
   *
   * @param type The @ref PropertyType.
   * @param componentType The @ref PropertyComponentType.
   * @param array Whether the value is an array type.
   */
  MetadataValueType(
      PropertyType type,
      PropertyComponentType inComponentType,
      bool array = false);

  /**
   * @brief Constructs a value type from the type of the given @ref
   * Cesium3DTiles::ClassProperty.
   *
   * @param property The @ref Cesium3DTiles::ClassProperty.
   */
  MetadataValueType(const Cesium3DTiles::ClassProperty& property);

  /**
   * @brief Constructs a value type from the type of the given @ref
   * CesiumGltf::ClassProperty.
   *
   * @param property The @ref CesiumGltf::ClassProperty.
   */
  MetadataValueType(const CesiumGltf::ClassProperty& property);

  inline bool operator==(const MetadataValueType& other) const {
    return this->type == other.type &&
           this->componentType == other.componentType &&
           this->array == other.array;
  }

  inline bool operator!=(const MetadataValueType& other) const {
    return !(*this == other);
  }
};

/**
 * @brief Gets the @ref MetadataValueType corresponding to the given C++ type.
 *
 * @tparam T The C++ type.
 */
template <typename T> static MetadataValueType getMetadataValueType() {
  MetadataValueType result;

  if constexpr (CesiumMetadata::IsMetadataArray<T>::value) {
    using ArrayType = typename CesiumMetadata::MetadataArrayType<T>::type;
    result.type = CesiumMetadata::TypeToPropertyType<ArrayType>::value;
    result.componentType =
        CesiumMetadata::TypeToPropertyType<ArrayType>::component;
    result.array = true;
  } else {
    result.type = CesiumMetadata::TypeToPropertyType<T>::value;
    result.componentType = CesiumMetadata::TypeToPropertyType<T>::component;
    result.array = false;
  }

  return result;
}

} // namespace CesiumMetadata
