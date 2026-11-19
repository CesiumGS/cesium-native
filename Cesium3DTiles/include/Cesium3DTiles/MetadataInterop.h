#pragma once

#include <CesiumUtility/Result.h>

namespace CesiumGltf {
struct Class;
struct ClassProperty;
struct ClassStatistics;
struct Enum;
struct EnumValue;
struct PropertyStatistics;
struct Schema;
struct Statistics;
} // namespace CesiumGltf

namespace CesiumTilesetJson {
struct Class;
struct ClassProperty;
struct ClassStatistics;
struct Enum;
struct EnumValue;
struct PropertyStatistics;
struct Schema;
struct Statistics;
} // namespace CesiumTilesetJson

namespace Cesium3DTiles {

/**
 * @brief Supports conversion of metadata elements between 3D Tiles 1.1 and 3D
 * Tiles 2.0 (glTF 2.1).
 */
class MetadataInterop {
public:
  /**
   * @brief Converts a @ref CesiumTilesetJson::ClassProperty to a @ref
   * CesiumGltf::ClassProperty.
   *
   * @param classProperty The @ref CesiumTilesetJson::ClassProperty to convert.
   * @returns The converted class property.
   */
  static CesiumGltf::ClassProperty
  convertClassProperty(CesiumTilesetJson::ClassProperty&& classProperty);

  /**
   * @brief Converts a @ref CesiumGltf::ClassProperty to a @ref
   * CesiumTilesetJson::ClassProperty.
   *
   * @param classProperty The @ref CesiumGltf::ClassProperty to convert.
   * @returns The converted class property.
   */
  static CesiumTilesetJson::ClassProperty
  convertClassProperty(CesiumGltf::ClassProperty&& classProperty);

  /**
   * @brief Converts a @ref CesiumTilesetJson::Class to a @ref
   * CesiumGltf::Class.
   *
   * @param class_ The @ref CesiumTilesetJson::Class to convert.
   * @returns The converted class.
   */
  static CesiumGltf::Class convertClass(CesiumTilesetJson::Class&& class_);

  /**
   * @brief Converts a @ref CesiumGltf::Class to a @ref
   * CesiumTilesetJson::Class.
   *
   * @param class_ The @ref CesiumGltf::Class to convert.
   * @returns The converted class.
   */
  static CesiumTilesetJson::Class convertClass(CesiumGltf::Class&& class_);

  /**
   * @brief Converts a @ref CesiumTilesetJson::EnumValue to a @ref
   * CesiumGltf::EnumValue.
   *
   * @param enumValue The @ref CesiumTilesetJson::EnumValue to convert.
   * @returns The converted enum value.
   */
  static CesiumGltf::EnumValue
  convertEnumValue(CesiumTilesetJson::EnumValue&& enumValue);

  /**
   * @brief Converts a @ref CesiumGltf::EnumValue to a @ref
   * CesiumTilesetJson::EnumValue.
   *
   * @param enumValue The @ref CesiumGltf::EnumValue to convert.
   * @returns The converted enum value.
   */
  static CesiumTilesetJson::EnumValue
  convertEnumValue(CesiumGltf::EnumValue&& enumValue);

  /**
   * @brief Converts a @ref CesiumTilesetJson::Enum to a @ref
   * CesiumGltf::Enum.
   *
   * @param enum_ The @ref CesiumTilesetJson::Enum to convert.
   * @returns The converted enum.
   */
  static CesiumGltf::Enum convertEnum(CesiumTilesetJson::Enum&& enum_);

  /**
   * @brief Converts a @ref CesiumGltf::Enum to a @ref
   * CesiumTilesetJson::Enum.
   *
   * @param enum_ The @ref CesiumGltf::Enum to convert.
   * @returns The converted enum.
   */
  static CesiumTilesetJson::Enum convertEnum(CesiumGltf::Enum&& enum_);

  /**
   * @brief Converts a @ref CesiumTilesetJson::Schema to a @ref
   * CesiumGltf::Schema.
   *
   * @param buffer The @ref CesiumTilesetJson::Schema to convert.
   * @returns The converted schema.
   */
  static CesiumGltf::Schema convertSchema(CesiumTilesetJson::Schema&& schema);

  /**
   * @brief Converts a @ref CesiumGltf::Schema to a @ref
   * CesiumTilesetJson::Schema.
   *
   * @param buffer The @ref CesiumGltf::Schema to convert.
   * @returns The converted schema.
   */
  static CesiumTilesetJson::Schema convertSchema(CesiumGltf::Schema&& schema);

  /**
   * @brief Converts a @ref CesiumTilesetJson::PropertyStatistics to a @ref
   * CesiumGltf::PropertyStatistics.
   *
   * @param propertyStatistics The @ref CesiumTilesetJson::PropertyStatistics to
   * convert.
   * @returns The converted property statistics.
   */
  static CesiumGltf::PropertyStatistics convertPropertyStatistics(
      CesiumTilesetJson::PropertyStatistics&& propertyStatistics);

  /**
   * @brief Converts a @ref CesiumGltf::PropertyStatistics to a @ref
   * CesiumTilesetJson::PropertyStatistics.
   *
   * @param propertyStatistics The @ref CesiumGltf::PropertyStatistics to
   * convert.
   * @returns The converted property statistics.
   */
  static CesiumTilesetJson::PropertyStatistics convertPropertyStatistics(
      CesiumGltf::PropertyStatistics&& propertyStatistics);

  /**
   * @brief Converts a @ref CesiumGltf::ClassStatistics to a @ref
   * CesiumTilesetJson::ClassStatistics.
   *
   * @param classStatistics The @ref CesiumGltf::ClassStatistics to convert.
   * @returns The converted class statistics.
   */
  static CesiumTilesetJson::ClassStatistics
  convertClassStatistics(CesiumGltf::ClassStatistics&& classStatistics);

  /**
   * @brief Converts a @ref CesiumTilesetJson::ClassStatistics to a @ref
   * CesiumGltf::ClassStatistics.
   *
   * @param classStatistics The @ref CesiumTilesetJson::ClassStatistics to
   * convert.
   * @returns The converted class statistics.
   */
  static CesiumGltf::ClassStatistics
  convertClassStatistics(CesiumTilesetJson::ClassStatistics&& classStatistics);

  /**
   * @brief Converts a @ref CesiumTilesetJson::Statistics to a @ref
   * CesiumGltf::Statistics.
   *
   * @param buffer The @ref CesiumTilesetJson::Statistics to convert.
   * @returns The converted statistics.
   */
  static CesiumGltf::Statistics
  convertStatistics(CesiumTilesetJson::Statistics&& statistics);

  /**
   * @brief Converts a @ref CesiumGltf::Statistics to a @ref
   * CesiumTilesetJson::Statistics.
   *
   * @param buffer The @ref CesiumGltf::Statistics to convert.
   * @returns The converted statistics.
   */
  static CesiumTilesetJson::Statistics
  convertStatistics(CesiumGltf::Statistics&& statistics);
};
} // namespace Cesium3DTiles
