#pragma once

#include "CesiumJsonWriter/ExtensionWriterContext.h"
#include "CesiumJsonWriter/JsonWriter.h"

#include <spdlog/fmt/fmt.h>

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
    auto handler = context.createExtensionHandler(
        item.first,
        item.second,
        TExtended::TypeName);
    if (!handler) {
      if (context.getExtensionState(item.first) != ExtensionState::Disabled) {
        jsonWriter.emplaceWarning(fmt::format(
            "Encountered unregistered extension {}. This extension will be "
            "ignored. To silence this warning, disable the extension with "
            "ExtensionWriterContext::setExtensionState.",
            item.first));
      }
      continue;
    }
    jsonWriter.Key(item.first);
    handler(item.second, jsonWriter, context);
  }
  jsonWriter.EndObject();
}
} // namespace CesiumJsonWriter
