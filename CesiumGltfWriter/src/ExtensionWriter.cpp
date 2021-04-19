#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include <CesiumUtility/JsonValue.h>

using namespace CesiumUtility;

void CesiumGltf::writeExtensions(
    const std::unordered_map<std::string, std::any>& extensions,
    CesiumGltf::JsonWriter& jsonWriter) {
  if (extensions.empty()) {
    return;
  }

  auto& j = jsonWriter;
  j.Key("extensions");
  j.StartObject();

  for (const auto& extension : extensions) {
    if (extension.second.type() == typeid(JsonValue)) {
      const auto& object = std::any_cast<JsonValue>(extension.second);
      j.Key(extension.first);
      writeJsonValue(object, jsonWriter);
    }
  }

  j.EndObject();
}
