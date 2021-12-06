#include "CesiumJsonReader/ExtensionReaderContext.h"

#include "CesiumJsonReader/IExtensionJsonHandler.h"
#include "CesiumJsonReader/JsonObjectJsonHandler.h"
#include "CesiumJsonReader/JsonReader.h"

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

  virtual IJsonHandler* readNull() override {
    return JsonObjectJsonHandler::readNull();
  };
  virtual IJsonHandler* readBool(bool b) override {
    return JsonObjectJsonHandler::readBool(b);
  }
  virtual IJsonHandler* readInt32(int32_t i) override {
    return JsonObjectJsonHandler::readInt32(i);
  }
  virtual IJsonHandler* readUint32(uint32_t i) override {
    return JsonObjectJsonHandler::readUint32(i);
  }
  virtual IJsonHandler* readInt64(int64_t i) override {
    return JsonObjectJsonHandler::readInt64(i);
  }
  virtual IJsonHandler* readUint64(uint64_t i) override {
    return JsonObjectJsonHandler::readUint64(i);
  }
  virtual IJsonHandler* readDouble(double d) override {
    return JsonObjectJsonHandler::readDouble(d);
  }
  virtual IJsonHandler* readString(const std::string_view& str) override {
    return JsonObjectJsonHandler::readString(str);
  }
  virtual IJsonHandler* readObjectStart() override {
    return JsonObjectJsonHandler::readObjectStart();
  }
  virtual IJsonHandler* readObjectKey(const std::string_view& str) override {
    return JsonObjectJsonHandler::readObjectKey(str);
  }
  virtual IJsonHandler* readObjectEnd() override {
    return JsonObjectJsonHandler::readObjectEnd();
  }
  virtual IJsonHandler* readArrayStart() override {
    return JsonObjectJsonHandler::readArrayStart();
  }
  virtual IJsonHandler* readArrayEnd() override {
    return JsonObjectJsonHandler::readArrayEnd();
  }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    JsonObjectJsonHandler::reportWarning(warning, std::move(context));
  }
};

void ExtensionReaderContext::setExtensionState(
    const std::string& extensionName,
    ExtensionState newState) {
  this->_extensionStates[extensionName] = newState;
}

std::unique_ptr<IExtensionJsonHandler>
ExtensionReaderContext::createExtensionHandler(
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
