#include "ExtensibleObjectJsonHandler.h"
#include "CesiumGltf/ExtensibleObject.h"

using namespace CesiumGltf;

void ExtensibleObjectJsonHandler::reset(
    IJsonHandler* pParent,
    ExtensibleObject* /*pObject*/) {
  ObjectJsonHandler::reset(pParent);
}

IJsonHandler* ExtensibleObjectJsonHandler::ExtensibleObjectKey(
    const char* str,
    ExtensibleObject& o) {
  using namespace std::string_literals;

  if ("extras"s == str)
    return property("extras", this->_extras, o.extras);

  if ("extensions"s == str) {
    o.extensions.emplace_back(JsonValue::Object());
    return property("extensions", this->_extensions, std::any_cast<JsonValue::Object&>(o.extensions.back()));
  }

  return this->ignoreAndContinue();
}