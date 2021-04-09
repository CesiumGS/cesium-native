#include "NamedObjectJsonHandler.h"
#include "CesiumGltf/NamedObject.h"
#include <ExtensibleObjectJsonHandler.h>
#include <string>

using namespace CesiumGltf;

NamedObjectJsonHandler::NamedObjectJsonHandler(
    const ReaderContext& context) noexcept
    : ExtensibleObjectJsonHandler(context), _name(context) {}

void NamedObjectJsonHandler::reset(
    IJsonReader* pParent,
    NamedObject* pObject) {
  ExtensibleObjectJsonHandler::reset(pParent, pObject);
}

IJsonReader* NamedObjectJsonHandler::readObjectKeyNamedObject(
    const std::string& objectType,
    const std::string_view& str,
    NamedObject& o) {
  using namespace std::string_literals;
  if ("name"s == str)
    return property("name", this->_name, o.name);
  return this->readObjectKeyExtensibleObject(objectType, str, o);
}
