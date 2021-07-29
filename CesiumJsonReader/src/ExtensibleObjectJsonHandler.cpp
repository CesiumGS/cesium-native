#include "CesiumJsonReader/ExtensibleObjectJsonHandler.h"
#include "CesiumJsonReader/ExtensionsJsonHandler.h"
#include "CesiumJsonReader/JsonHandler.h"
#include "CesiumJsonReader/ObjectJsonHandler.h"

using namespace CesiumJsonReader;
using namespace CesiumUtility;

ExtensibleObjectJsonHandler::ExtensibleObjectJsonHandler(
    const ExtensionReaderContext& context) noexcept
    : ObjectJsonHandler(), _extras(), _extensions(context) {}

void ExtensibleObjectJsonHandler::reset(
    IJsonHandler* pParent,
    ExtensibleObject* /*pObject*/) {
  ObjectJsonHandler::reset(pParent);
}

IJsonHandler* ExtensibleObjectJsonHandler::readObjectKeyExtensibleObject(
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
