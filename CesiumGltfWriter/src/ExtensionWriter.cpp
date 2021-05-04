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
    const auto isObject = extension.second.type() == typeid(JsonValue::Object);
    const auto isArray = extension.second.type() == typeid(JsonValue::Array);
    const auto isPrimitive = extension.second.type() == typeid(JsonValue);

    // Always assume we're inside of an object, ExtensibleObject::extensions
    // forces extensions to be in a key / value setup.
    j.Key(extension.first);

    if (isObject) {
      const auto& object = std::any_cast<JsonValue::Object>(extension.second);
      writeJsonValue(object, jsonWriter);
    }

    else if (isArray) {
      const auto& array = std::any_cast<JsonValue::Array>(extension.second);
      writeJsonValue(array, jsonWriter);
    }

    else if (isPrimitive) {
      const auto& primitive = std::any_cast<JsonValue>(extension.second);
      writeJsonValue(primitive, jsonWriter);
    }
  }

  j.EndObject();
}
