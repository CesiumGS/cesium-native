#include <CesiumJsonReader/IExtensionJsonHandler.h>
#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/JsonValue.h>

#include <any>
#include <memory>
#include <string>
#include <string_view>

namespace CesiumJsonReader {
class AnyExtensionJsonHandler : public JsonObjectJsonHandler,
                                public IExtensionJsonHandler {
public:
  AnyExtensionJsonHandler() noexcept : JsonObjectJsonHandler() {}

  virtual void reset(
      IJsonHandler* pParentHandler,
      CesiumUtility::ExtensibleObject& o,
      const std::string_view& extensionName) override {
    std::any& value =
        o.extensions
            .emplace(
                extensionName,
                CesiumUtility::JsonValue(CesiumUtility::JsonValue::Object()))
            .first->second;
    JsonObjectJsonHandler::reset(
        pParentHandler,
        &std::any_cast<CesiumUtility::JsonValue&>(value));
  }

  virtual IJsonHandler& getHandler() override { return *this; }
};

ExtensionState
JsonReaderOptions::getExtensionState(const std::string& extensionName) const {
  auto stateIt = this->_extensionStates.find(extensionName);
  if (stateIt == this->_extensionStates.end()) {
    return ExtensionState::Enabled;
  }

  return stateIt->second;
}

void JsonReaderOptions::setExtensionState(
    const std::string& extensionName,
    ExtensionState newState) {
  this->_extensionStates[extensionName] = newState;
}

std::unique_ptr<IExtensionJsonHandler>
JsonReaderOptions::createExtensionHandler(
    const std::string_view& extensionName,
    const std::string& extendedObjectType) const {

  std::string extensionNameString{extensionName};

  auto stateIt = this->_extensionStates.find(extensionNameString);
  if (stateIt != this->_extensionStates.end()) {
    if (stateIt->second == ExtensionState::Disabled) {
      return nullptr;
    } else if (stateIt->second == ExtensionState::JsonOnly) {
      return std::make_unique<AnyExtensionJsonHandler>();
    }
  }

  auto extensionNameIt = this->_extensions.find(extensionNameString);
  if (extensionNameIt == this->_extensions.end()) {
    return std::make_unique<AnyExtensionJsonHandler>();
  }

  auto objectTypeIt = extensionNameIt->second.find(extendedObjectType);
  if (objectTypeIt == extensionNameIt->second.end()) {
    return std::make_unique<AnyExtensionJsonHandler>();
  }

  return objectTypeIt->second(*this);
}
} // namespace CesiumJsonReader
