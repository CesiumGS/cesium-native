#include "CesiumJsonWriter/ExtensionWriterContext.h"
#include "CesiumJsonWriter/JsonWriter.h"

using namespace CesiumJsonWriter;

namespace {
void noOpWriter(const std::any&, JsonWriter&, const ExtensionWriterContext&) {}
} // namespace

ExtensionWriterContext::ExtensionHandler<std::any>
ExtensionWriterContext::createExtensionHandler(
    const std::string_view& extensionName,
    const std::string& extendedObjectType) const {

  std::string extensionNameString{extensionName};

  auto stateIt = this->_extensionStates.find(extensionNameString);
  if (stateIt != this->_extensionStates.end()) {
    if (stateIt->second == ExtensionState::Disabled) {
      return noOpWriter;
    }
  }

  auto extensionNameIt = this->_extensions.find(extensionNameString);
  if (extensionNameIt == this->_extensions.end()) {
    return noOpWriter;
  }

  auto objectTypeIt = extensionNameIt->second.find(extendedObjectType);
  if (objectTypeIt == extensionNameIt->second.end()) {
    return noOpWriter;
  }

  return objectTypeIt->second;
}
