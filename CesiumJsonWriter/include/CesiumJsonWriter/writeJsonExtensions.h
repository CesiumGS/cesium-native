#pragma once

#include "CesiumJsonWriter/ExtensionWriterContext.h"
#include "CesiumJsonWriter/JsonWriter.h"

namespace CesiumJsonWriter {
template <typename TExtended>
void writeJsonExtensions(
    const TExtended& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  if (obj.extensions.empty()) {
    return;
  }
  jsonWriter.StartObject();
  for (const auto& item : obj.extensions) {
    auto handler =
        context.createExtensionHandler(item.first, TExtended::TypeName);
    if (!handler) {
      continue;
    }
    jsonWriter.Key(item.first);
    handler(item.second, jsonWriter, context);
  }
  jsonWriter.EndObject();
}
} // namespace CesiumJsonWriter
