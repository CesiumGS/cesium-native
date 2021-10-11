#include "ExtensibleObjectJsonHandler.h"

#include "CesiumGltf/ExtensibleObject.h"
#include "CesiumJsonReader/JsonHandler.h"
#include "CesiumJsonReader/ObjectJsonHandler.h"
#include "ExtensionsJsonHandler.h"

using namespace CesiumGltf;
using namespace CesiumJsonReader;

ExtensibleObjectJsonHandler::ExtensibleObjectJsonHandler(
    const ReaderContext& context) noexcept
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
