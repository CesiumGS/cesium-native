#include "ExtensionWriter.h"

#include "CesiumGltfWriter/ExtensionMaterialsUnlit.h"

#include <CesiumGltf/ExtensionMaterialsUnlit.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumUtility/JsonValue.h>

namespace CesiumGltfWriter {
void writeExtensions(
    const std::map<std::string, std::any>& extensions,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  if (extensions.empty()) {
    return;
  }

  auto& j = jsonWriter;
  j.Key("extensions");
  j.StartObject();

  for (const auto& extension : extensions) {
    const auto isObject =
        extension.second.type() == typeid(CesiumUtility::JsonValue::Object);
    const auto isArray =
        extension.second.type() == typeid(CesiumUtility::JsonValue::Array);
    const auto isPrimitive =
        extension.second.type() == typeid(CesiumUtility::JsonValue);
    const auto isExtensionMaterialsUnlit =
        extension.second.type() == typeid(CesiumGltf::ExtensionMaterialsUnlit);

    // Always assume we're inside of an object, ExtensibleObject::extensions
    // forces extensions to be in a key / value setup.
    j.Key(extension.first);

    if (isObject) {
      const auto& object =
          std::any_cast<CesiumUtility::JsonValue::Object>(extension.second);
      writeJsonValue(object, jsonWriter);
    }

    else if (isArray) {
      const auto& array =
          std::any_cast<CesiumUtility::JsonValue::Array>(extension.second);
      writeJsonValue(array, jsonWriter);
    }

    else if (isPrimitive) {
      const auto& primitive =
          std::any_cast<CesiumUtility::JsonValue>(extension.second);
      writeJsonValue(primitive, jsonWriter);
    }

    else if (isExtensionMaterialsUnlit) {
      const auto& primitive =
          std::any_cast<CesiumGltf::ExtensionMaterialsUnlit>(extension.second);
      writeMaterialsUnlit(primitive, jsonWriter);
    }
  }

  j.EndObject();
}

} // namespace CesiumGltfWriter
