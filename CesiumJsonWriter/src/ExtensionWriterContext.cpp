#include "CesiumJsonWriter/ExtensionWriterContext.h"

#include "CesiumJsonWriter/IExtensionJsonHandler.h"
#include "CesiumJsonWriter/JsonObjectJsonHandler.h"
#include "CesiumJsonWriter/JsonWriter.h"

using namespace CesiumJsonWriter;
using namespace CesiumUtility;

class AnyExtensionJsonHandler : public JsonObjectJsonHandler,
                                public IExtensionJsonHandler {
public:
  AnyExtensionJsonHandler() noexcept : JsonObjectJsonHandler() {}

  virtual void reset(
      IJsonHandler* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) override {
    std::any& value =
        o.extensions.emplace(extensionName, JsonValue(JsonValue::Object()))
            .first->second;
    JsonObjectJsonHandler::reset(
        pParentHandler,
        &std::any_cast<JsonValue&>(value));
  }

  virtual IJsonHandler* writeNull() override {
    return JsonObjectJsonHandler::writeNull();
  };
  virtual IJsonHandler* writeBool(bool b) override {
    return JsonObjectJsonHandler::writeBool(b);
  }
  virtual IJsonHandler* writeInt32(int32_t i) override {
    return JsonObjectJsonHandler::writeInt32(i);
  }
  virtual IJsonHandler* writeUint32(uint32_t i) override {
    return JsonObjectJsonHandler::writeUint32(i);
  }
  virtual IJsonHandler* writeInt64(int64_t i) override {
    return JsonObjectJsonHandler::writeInt64(i);
  }
  virtual IJsonHandler* writeUint64(uint64_t i) override {
    return JsonObjectJsonHandler::writeUint64(i);
  }
  virtual IJsonHandler* writeDouble(double d) override {
    return JsonObjectJsonHandler::writeDouble(d);
  }
  virtual IJsonHandler* writeString(const std::string_view& str) override {
    return JsonObjectJsonHandler::writeString(str);
  }
  virtual IJsonHandler* writeObjectStart() override {
    return JsonObjectJsonHandler::writeObjectStart();
  }
  virtual IJsonHandler* writeObjectKey(const std::string_view& str) override {
    return JsonObjectJsonHandler::writeObjectKey(str);
  }
  virtual IJsonHandler* writeObjectEnd() override {
    return JsonObjectJsonHandler::writeObjectEnd();
  }
  virtual IJsonHandler* writeArrayStart() override {
    return JsonObjectJsonHandler::writeArrayStart();
  }
  virtual IJsonHandler* writeArrayEnd() override {
    return JsonObjectJsonHandler::writeArrayEnd();
  }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    JsonObjectJsonHandler::reportWarning(warning, std::move(context));
  }
};

void ExtensionWriterContext::setExtensionState(
    const std::string& extensionName,
    ExtensionState newState) {
  this->_extensionStates[extensionName] = newState;
}

std::unique_ptr<IExtensionJsonHandler>
ExtensionWriterContext::createExtensionHandler(
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
