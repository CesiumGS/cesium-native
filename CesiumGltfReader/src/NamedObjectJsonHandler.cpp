#include "NamedObjectJsonHandler.h"

#include <CesiumGltf/NamedObject.h>

#include <ExtensibleObjectJsonHandler.h>

#include <string>

using namespace CesiumGltf;

NamedObjectJsonHandler::NamedObjectJsonHandler(
    const ReaderContext& context) noexcept
    : ExtensibleObjectJsonHandler(context), _name() {}

void NamedObjectJsonHandler::reset(
    IJsonHandler* pParent,
    NamedObject* pObject) {
  ExtensibleObjectJsonHandler::reset(pParent, pObject);
}

CesiumJsonReader::IJsonHandler*
NamedObjectJsonHandler::readObjectKeyNamedObject(
    const std::string& objectType,
    const std::string_view& str,
    NamedObject& o) {
  using namespace std::string_literals;
  if ("name"s == str)
    return property("name", this->_name, o.name);
  return this->readObjectKeyExtensibleObject(objectType, str, o);
}
