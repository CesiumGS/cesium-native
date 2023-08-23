#include <Cesium3DTiles/MetadataQuery.h>

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

  for (auto it = entity.properties.begin(); it != entity.properties.end();
       ++it) {
    const std::pair<std::string, CesiumUtility::JsonValue>& property = *it;
    auto propertyIt = klass.properties.find(property.first);
    if (propertyIt == klass.properties.end())
      continue;

    const ClassProperty& classProperty = propertyIt->second;
    if (classProperty.semantic == semantic) {
      return FoundMetadataProperty{
          classIt->first,
          classIt->second,
          it->first,
          propertyIt->second,
          it->second};
    }
  }

  return std::nullopt;
}

} // namespace Cesium3DTiles
