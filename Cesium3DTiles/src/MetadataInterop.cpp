#include "moveExtensibleObjectProperties.h"

#include <Cesium3DTiles/MetadataInterop.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltf/Statistics.h>
#include <CesiumTilesetJson/Class.h>
#include <CesiumTilesetJson/ClassProperty.h>
#include <CesiumTilesetJson/Schema.h>
#include <CesiumTilesetJson/Statistics.h>

#include <utility>

namespace Cesium3DTiles {

CesiumGltf::ClassProperty MetadataInterop::convertClassProperty(
    CesiumTilesetJson::ClassProperty&& classProperty) {
  CesiumGltf::ClassProperty result;
  result.name = std::move(classProperty.name);
  result.description = std::move(classProperty.description);

  result.type = std::move(classProperty.type);
  result.componentType = std::move(classProperty.componentType);
  result.enumType = std::move(classProperty.enumType);
  result.array = classProperty.array;
  result.count = classProperty.count;
  result.normalized = classProperty.normalized;

  result.offset = std::move(classProperty.offset);
  result.scale = std::move(classProperty.scale);
  result.min = std::move(classProperty.min);
  result.max = std::move(classProperty.max);
  result.required = classProperty.required;
  result.noData = std::move(classProperty.noData);
  result.defaultProperty = std::move(classProperty.defaultProperty);

  result.semantic = std::move(classProperty.semantic);

  moveExtensibleObjectProperties(result, classProperty);
  return result;
}

CesiumTilesetJson::ClassProperty MetadataInterop::convertClassProperty(
    CesiumGltf::ClassProperty&& classProperty) {
  CesiumTilesetJson::ClassProperty result;
  result.name = std::move(classProperty.name);
  result.description = std::move(classProperty.description);

  result.type = std::move(classProperty.type);
  result.componentType = std::move(classProperty.componentType);
  result.enumType = std::move(classProperty.enumType);
  result.array = classProperty.array;
  result.count = classProperty.count;
  result.normalized = classProperty.normalized;

  result.offset = std::move(classProperty.offset);
  result.scale = std::move(classProperty.scale);
  result.min = std::move(classProperty.min);
  result.max = std::move(classProperty.max);
  result.required = classProperty.required;
  result.noData = std::move(classProperty.noData);
  result.defaultProperty = std::move(classProperty.defaultProperty);

  result.semantic = std::move(classProperty.semantic);

  moveExtensibleObjectProperties(result, classProperty);
  return result;
}

CesiumGltf::Class
MetadataInterop::convertClass(CesiumTilesetJson::Class&& class_) {
  CesiumGltf::Class result;
  result.name = std::move(class_.name);
  result.description = std::move(class_.description);
  for (auto& [propertyId, classProperty] : class_.properties) {
    result.properties.emplace(
        propertyId,
        convertClassProperty(std::move(classProperty)));
  }
  result.parent = std::move(class_.parent);

  moveExtensibleObjectProperties(result, class_);
  return result;
}

CesiumTilesetJson::Class
MetadataInterop::convertClass(CesiumGltf::Class&& class_) {
  CesiumTilesetJson::Class result;
  result.name = std::move(class_.name);
  result.description = std::move(class_.description);

  result.properties.reserve(class_.properties.size());
  for (auto& [propertyId, classProperty] : class_.properties) {
    result.properties.emplace(
        propertyId,
        convertClassProperty(std::move(classProperty)));
  }

  result.parent = std::move(class_.parent);

  moveExtensibleObjectProperties(result, class_);
  return result;
}

CesiumGltf::EnumValue
MetadataInterop::convertEnumValue(CesiumTilesetJson::EnumValue&& enumValue) {
  CesiumGltf::EnumValue result;
  result.name = std::move(enumValue.name);
  result.description = std::move(enumValue.description);
  result.value = enumValue.value;
  moveExtensibleObjectProperties(result, enumValue);
  return result;
}

CesiumTilesetJson::EnumValue
MetadataInterop::convertEnumValue(CesiumGltf::EnumValue&& enumValue) {
  CesiumTilesetJson::EnumValue result;
  result.name = std::move(enumValue.name);
  result.description = std::move(enumValue.description);
  result.value = enumValue.value;
  moveExtensibleObjectProperties(result, enumValue);
  return result;
}

CesiumGltf::Enum MetadataInterop::convertEnum(CesiumTilesetJson::Enum&& enum_) {
  CesiumGltf::Enum result;
  result.name = std::move(enum_.name);
  result.description = std::move(enum_.description);
  result.valueType = std::move(enum_.valueType);

  result.values.reserve(enum_.values.size());
  for (size_t i = 0; i < enum_.values.size(); i++) {
    result.values.emplace_back(convertEnumValue(std::move(enum_.values[i])));
  }
  moveExtensibleObjectProperties(result, enum_);
  return result;
}

CesiumTilesetJson::Enum MetadataInterop::convertEnum(CesiumGltf::Enum&& enum_) {
  CesiumTilesetJson::Enum result;
  result.name = std::move(enum_.name);
  result.description = std::move(enum_.description);
  result.valueType = std::move(enum_.valueType);

  result.values.reserve(enum_.values.size());
  for (size_t i = 0; i < enum_.values.size(); i++) {
    result.values.emplace_back(convertEnumValue(std::move(enum_.values[i])));
  }
  moveExtensibleObjectProperties(result, enum_);
  return result;
}

CesiumGltf::Schema
MetadataInterop::convertSchema(CesiumTilesetJson::Schema&& schema) {
  CesiumGltf::Schema result;
  result.id = std::move(schema.id);
  result.name = std::move(schema.name);
  result.description = std::move(schema.description);
  result.version = std::move(schema.version);
  for (auto& [classId, class_] : schema.classes) {
    result.classes.emplace(classId, convertClass(std::move(class_)));
  }
  for (auto& [enumId, enum_] : schema.enums) {
    result.enums.emplace(enumId, convertEnum(std::move(enum_)));
  }
  moveExtensibleObjectProperties(result, schema);
  return result;
}

CesiumTilesetJson::Schema
MetadataInterop::convertSchema(CesiumGltf::Schema&& schema) {
  CesiumTilesetJson::Schema result;
  result.id = std::move(schema.id);
  result.name = std::move(schema.name);
  result.description = std::move(schema.description);
  result.version = std::move(schema.version);
  for (auto& [classId, class_] : schema.classes) {
    result.classes.emplace(classId, convertClass(std::move(class_)));
  }
  for (auto& [enumId, enum_] : schema.enums) {
    result.enums.emplace(enumId, convertEnum(std::move(enum_)));
  }
  moveExtensibleObjectProperties(result, schema);
  return result;
}

CesiumGltf::PropertyStatistics MetadataInterop::convertPropertyStatistics(
    CesiumTilesetJson::PropertyStatistics&& propertyStatistics) {
  CesiumGltf::PropertyStatistics result;
  result.min = std::move(propertyStatistics.min);
  result.max = std::move(propertyStatistics.max);
  result.mean = std::move(propertyStatistics.mean);
  result.median = std::move(propertyStatistics.median);
  result.standardDeviation = std::move(propertyStatistics.standardDeviation);
  result.variance = std::move(propertyStatistics.variance);
  result.sum = std::move(propertyStatistics.sum);
  result.occurrences = std::move(propertyStatistics.occurrences);
  moveExtensibleObjectProperties(result, propertyStatistics);
  return result;
}

CesiumTilesetJson::PropertyStatistics
MetadataInterop::convertPropertyStatistics(
    CesiumGltf::PropertyStatistics&& propertyStatistics) {
  CesiumTilesetJson::PropertyStatistics result;
  result.min = std::move(propertyStatistics.min);
  result.max = std::move(propertyStatistics.max);
  result.mean = std::move(propertyStatistics.mean);
  result.median = std::move(propertyStatistics.median);
  result.standardDeviation = std::move(propertyStatistics.standardDeviation);
  result.variance = std::move(propertyStatistics.variance);
  result.sum = std::move(propertyStatistics.sum);
  result.occurrences = std::move(propertyStatistics.occurrences);
  moveExtensibleObjectProperties(result, propertyStatistics);
  return result;
}

CesiumGltf::ClassStatistics MetadataInterop::convertClassStatistics(
    CesiumTilesetJson::ClassStatistics&& classStatistics) {
  CesiumGltf::ClassStatistics result;
  result.count = classStatistics.count;
  result.properties.reserve(classStatistics.properties.size());
  for (auto& [propertyId, propertyStatistics] : classStatistics.properties) {
    result.properties.emplace(
        propertyId,
        convertPropertyStatistics(std::move(propertyStatistics)));
  }
  moveExtensibleObjectProperties(result, classStatistics);
  return result;
}

CesiumTilesetJson::ClassStatistics MetadataInterop::convertClassStatistics(
    CesiumGltf::ClassStatistics&& classStatistics) {
  CesiumTilesetJson::ClassStatistics result;
  result.count = classStatistics.count;
  result.properties.reserve(classStatistics.properties.size());
  for (auto& [propertyId, propertyStatistics] : classStatistics.properties) {
    result.properties.emplace(
        propertyId,
        convertPropertyStatistics(std::move(propertyStatistics)));
  }
  moveExtensibleObjectProperties(result, classStatistics);
  return result;
}

CesiumGltf::Statistics
MetadataInterop::convertStatistics(CesiumTilesetJson::Statistics&& statistics) {
  CesiumGltf::Statistics result;
  result.classes.reserve(statistics.classes.size());
  for (auto& [classId, classStatistics] : statistics.classes) {
    result.classes.emplace(
        classId,
        convertClassStatistics(std::move(classStatistics)));
  }
  moveExtensibleObjectProperties(result, statistics);
  return result;
}

CesiumTilesetJson::Statistics
MetadataInterop::convertStatistics(CesiumGltf::Statistics&& statistics) {
  CesiumTilesetJson::Statistics result;
  result.classes.reserve(statistics.classes.size());
  for (auto& [classId, classStatistics] : statistics.classes) {
    result.classes.emplace(
        classId,
        convertClassStatistics(std::move(classStatistics)));
  }
  moveExtensibleObjectProperties(result, statistics);
  return result;
}

} // namespace Cesium3DTiles
