#include "ExtensibleObjectJsonHandler.h"
#include "CesiumGltf/ExtensibleObject.h"
#include "CesiumJsonReader/JsonReader.h"
#include "CesiumJsonReader/ObjectJsonHandler.h"
#include "ExtensionsJsonHandler.h"

using namespace CesiumGltf;

ExtensibleObjectJsonHandler::ExtensibleObjectJsonHandler(
    const ReaderContext& context) noexcept
    : ObjectJsonHandler(), _extras(), _extensions(context) {}

void ExtensibleObjectJsonHandler::reset(
    IJsonReader* pParent,
    ExtensibleObject* /*pObject*/) {
  ObjectJsonHandler::reset(pParent);
}

IJsonReader* ExtensibleObjectJsonHandler::readObjectKeyExtensibleObject(
    const std::string& objectType,
    const std::string_view& str,
    ExtensibleObject& o) {
  using namespace std::string_literals;

  if ("extras"s == str)
    return property("extras", this->_extras, o.extras);

  if ("extensions"s == str) {
    this->_extensions.reset(this, &o, objectType);
    return &this->_extensions;
  }

  return this->ignoreAndContinue();
}
