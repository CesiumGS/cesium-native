#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumUtility/JsonValue.h>

#include <any>
#include <string>
#include <string_view>

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

ExtensionState ExtensionWriterContext::getExtensionState(
    const std::string& extensionName) const {
  auto stateIt = this->_extensionStates.find(extensionName);
  if (stateIt == this->_extensionStates.end()) {
    return ExtensionState::Enabled;
  }

  return stateIt->second;
}

void ExtensionWriterContext::setExtensionState(
    const std::string& extensionName,
    ExtensionState newState) {
  this->_extensionStates[extensionName] = newState;
}

ExtensionWriterContext::ExtensionHandler<std::any>
ExtensionWriterContext::createExtensionHandler(
    const std::string_view& extensionName,
    const std::any& obj,
    const std::string& extendedObjectType) const {

  std::string extensionNameString{extensionName};

  auto stateIt = this->_extensionStates.find(extensionNameString);
  if (stateIt != this->_extensionStates.end()) {
    if (stateIt->second == ExtensionState::Disabled) {
      return nullptr;
    }
  }

  if (std::any_cast<CesiumUtility::JsonValue>(&obj) != nullptr) {
    return objWriter;
  }

  auto extensionNameIt = this->_extensions.find(extensionNameString);
  if (extensionNameIt == this->_extensions.end()) {
    return nullptr;
  }

  auto objectTypeIt = extensionNameIt->second.find(extendedObjectType);
  if (objectTypeIt == extensionNameIt->second.end()) {
    return nullptr;
  }

  return objectTypeIt->second;
}
} // namespace CesiumJsonWriter
