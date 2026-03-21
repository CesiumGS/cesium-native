#pragma once

#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <spdlog/fmt/fmt.h>

namespace CesiumJsonWriter {

/**
 * @brief Writes the extensions attached to the provided object as a new JSON
 * object.
 *
 * @tparam TExtended The type with extensions attached to write. This type must
 * have an `extensions` property of type `std::unordered_map<std::string,
 * std::any>` or a similar type with an `empty()` method that returns pairs of
 * `<std::string, std::any>` when iterated.
 * @param obj The object with extensions to write.
 * @param jsonWriter The writer to write the extensions object to.
 * @param context The \ref ExtensionWriterContext that provides information on
 * how to write each extension.
 */
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
    if (handler) {
      jsonWriter.Key(item.first);
      handler(item.second, jsonWriter, context);
    }
  }
  jsonWriter.EndObject();
}

/**
 * @brief Checks if the provided object has any extensions attached that have
 * been registered with the provided \ref ExtensionWriterContext.
 *
 * @tparam TExtended The type with extensions attached to write. This type must
 * have an `extensions` property of type `std::unordered_map<std::string,
 * std::any>` or a similar type with an `empty()` method that returns pairs of
 * `<std::string, std::any>` when iterated.
 * @param obj The object with extensions to write.
 * @param jsonWriter The writer to write the extensions object to.
 * @param context The \ref ExtensionWriterContext with registered extensions.
 * @returns True if any extensions attached to `obj` have been registered, false
 * otherwise.
 */
template <typename TExtended>
bool hasRegisteredExtensions(
    const TExtended& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  bool hasRegisteredExtensions = false;

  for (const auto& item : obj.extensions) {
    auto handler = context.createExtensionHandler(
        item.first,
        item.second,
        TExtended::TypeName);
    if (handler) {
      hasRegisteredExtensions = true;
    } else if (
        context.getExtensionState(item.first) != ExtensionState::Disabled) {
      jsonWriter.emplaceWarning(fmt::format(
          "Encountered unregistered extension {}. This extension will be "
          "ignored. To silence this warning, disable the extension with "
          "ExtensionWriterContext::setExtensionState.",
          item.first));
    }
  }

  return hasRegisteredExtensions;
}
} // namespace CesiumJsonWriter
