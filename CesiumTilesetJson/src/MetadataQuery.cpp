#include <CesiumTilesetJson/Class.h>
#include <CesiumTilesetJson/ClassProperty.h>
#include <CesiumTilesetJson/MetadataEntity.h>
#include <CesiumTilesetJson/MetadataQuery.h>
#include <CesiumTilesetJson/Schema.h>

#include <optional>
#include <string>
#include <utility>

namespace CesiumTilesetJson {

std::optional<FoundMetadataProperty>
MetadataQuery::findFirstPropertyWithSemantic(
    const Schema& schema,
    const MetadataEntity& entity,
    const std::string& semantic) {
  auto classIt = schema.classes.find(entity.classProperty);
  if (classIt == schema.classes.end()) {
    return std::nullopt;
  }

  const CesiumTilesetJson::Class& klass = classIt->second;

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

} // namespace CesiumTilesetJson
