#pragma once

#include "CesiumJsonWriter/ExtensionWriterContext.h"
#include "CesiumJsonWriter/JsonWriter.h"

namespace CesiumJsonWriter {
template <typename TExtended>
void writeJsonExtensions(
    const TExtended& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  bool startedObject = false;

  for (const auto& item : obj.extensions) {
    auto handler =
        context.createExtensionHandler(item.first, TExtended::TypeName);
    if (!handler) {
      continue;
    }
    if (!startedObject) {
      jsonWriter.Key("extensions");
      jsonWriter.StartObject();
      startedObject = true;
    }
    jsonWriter.Key(item.first);
    handler(item.second, jsonWriter, context);
  }
  if (startedObject) {
    jsonWriter.EndObject();
  }
}
} // namespace CesiumJsonWriter
