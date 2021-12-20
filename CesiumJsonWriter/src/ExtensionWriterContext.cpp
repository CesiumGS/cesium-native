#include "CesiumJsonWriter/ExtensionWriterContext.h"

#include "CesiumJsonWriter/JsonObjectWriter.h"
#include "CesiumJsonWriter/JsonWriter.h"

#include <CesiumUtility/JsonValue.h>

namespace CesiumJsonWriter {
namespace {
void objWriter(
    const std::any& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& /* context */) {
  writeJsonValue(
      std::any_cast<const CesiumUtility::JsonValue&>(obj),
      jsonWriter);
}
} // namespace

ExtensionWriterContext::ExtensionHandler<std::any>
ExtensionWriterContext::createExtensionHandler(
    const std::string_view& extensionName,
    const std::string& extendedObjectType) const {

  std::string extensionNameString{extensionName};

  auto stateIt = this->_extensionStates.find(extensionNameString);
  if (stateIt != this->_extensionStates.end()) {
    if (stateIt->second == ExtensionState::Disabled) {
      return nullptr;
    } else if (stateIt->second == ExtensionState::JsonOnly) {
      return objWriter;
    }
  }

  auto extensionNameIt = this->_extensions.find(extensionNameString);
  if (extensionNameIt == this->_extensions.end()) {
    return objWriter;
  }

  auto objectTypeIt = extensionNameIt->second.find(extendedObjectType);
  if (objectTypeIt == extensionNameIt->second.end()) {
    return objWriter;
  }

  return objectTypeIt->second;
}
} // namespace CesiumJsonWriter
