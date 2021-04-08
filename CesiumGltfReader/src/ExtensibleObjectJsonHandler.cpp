#include "ExtensibleObjectJsonHandler.h"
#include "CesiumGltf/ExtensibleObject.h"
#include "CesiumGltf/JsonReader.h"
#include "ExtensionsJsonHandler.h"
#include "ObjectJsonHandler.h"

using namespace CesiumGltf;

ExtensibleObjectJsonHandler::ExtensibleObjectJsonHandler(
    const JsonReaderContext& context) noexcept
    : ObjectJsonHandler(context), _extras(context), _extensions(context) {}

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
