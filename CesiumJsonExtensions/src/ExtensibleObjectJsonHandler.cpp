#include "CesiumJsonExtensions/ExtensibleObjectJsonHandler.h"
#include "CesiumJsonExtensions/ExtensibleObject.h"
#include "CesiumJsonExtensions/ExtensionsJsonHandler.h"
#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>

using namespace CesiumJsonExtensions;
using namespace CesiumJsonReader;

ExtensibleObjectJsonHandler::ExtensibleObjectJsonHandler(
    const ExtensionContext& context) noexcept
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
