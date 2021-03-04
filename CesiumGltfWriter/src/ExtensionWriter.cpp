#include <CesiumGltf/JsonValue.h>
#include "JsonObjectWriter.h"
#include "ExtensionWriter.h"

void CesiumGltf::writeExtensions(
    const std::vector<std::any>& extensions, 
    CesiumGltf::JsonWriter& jsonWriter
) {
    if (extensions.empty()) {
        return;
    }
    
    auto& j = jsonWriter;
    j.Key("extensions");
    j.StartArray();

    for (const auto& extension : extensions) {
        if (extension.type() == typeid(JsonValue)) {
            const auto& object = std::any_cast<JsonValue>(extension);
            writeJsonValue(object, jsonWriter);
        }
    }

    j.EndArray();
}