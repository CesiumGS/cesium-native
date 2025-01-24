#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/ExtensionsJsonHandler.h>
#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <string>
#include <string_view>

namespace CesiumJsonReader {
ExtensibleObjectJsonHandler::ExtensibleObjectJsonHandler(
    const JsonReaderOptions& context) noexcept
    : ObjectJsonHandler(),
      _extras(),
      _extensions(context),
      _captureUnknownProperties(context.getCaptureUnknownProperties()) {}

void ExtensibleObjectJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumUtility::ExtensibleObject* /*pObject*/) {
  ObjectJsonHandler::reset(pParent);
}

IJsonHandler* ExtensibleObjectJsonHandler::readObjectKeyExtensibleObject(
    const std::string& objectType,
    const std::string_view& str,
    CesiumUtility::ExtensibleObject& o) {
  using namespace std::string_literals;

  if ("extras"s == str)
    return property("extras", this->_extras, o.extras);

  if ("extensions"s == str) {
    this->_extensions.reset(this, &o, objectType);
    return &this->_extensions;
  }

  if (this->_captureUnknownProperties) {
    // Add this unknown property to unknownProperties.
    auto it =
        o.unknownProperties.emplace(str, CesiumUtility::JsonValue()).first;
    this->setCurrentKey(it->first.c_str());
    CesiumUtility::JsonValue& value = it->second;
    this->_unknownProperties.reset(this, &value);
    return &this->_unknownProperties;
  } else {
    // Ignore this unknown property.
    return this->ignoreAndContinue();
  }
}
} // namespace CesiumJsonReader
