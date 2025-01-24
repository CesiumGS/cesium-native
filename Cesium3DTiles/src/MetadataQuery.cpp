#include <Cesium3DTiles/Class.h>
#include <Cesium3DTiles/ClassProperty.h>
#include <Cesium3DTiles/MetadataEntity.h>
#include <Cesium3DTiles/MetadataQuery.h>
#include <Cesium3DTiles/Schema.h>

#include <optional>
#include <string>
#include <utility>

namespace Cesium3DTiles {

std::optional<FoundMetadataProperty>
MetadataQuery::findFirstPropertyWithSemantic(
    const Schema& schema,
    const MetadataEntity& entity,
    const std::string& semantic) {
  auto classIt = schema.classes.find(entity.classProperty);
  if (classIt == schema.classes.end()) {
    return std::nullopt;
  }

  const Cesium3DTiles::Class& klass = classIt->second;

  for (const auto& propertyValue : entity.properties) {
    auto propertyIt = klass.properties.find(propertyValue.first);
    if (propertyIt == klass.properties.end())
      continue;

    const ClassProperty& classProperty = propertyIt->second;
    if (classProperty.semantic == semantic) {
      return FoundMetadataProperty{
          classIt->first,
          classIt->second,
          propertyValue.first,
          propertyIt->second,
          propertyValue.second};
    }
  }

  return std::nullopt;
}

} // namespace Cesium3DTiles
