#include "NamedObjectJsonHandler.h"

#include <CesiumGltf/NamedObject.h>
#include <CesiumJsonWriter/ExtensibleObjectJsonHandler.h>

#include <string>

using namespace CesiumGltf;

NamedObjectJsonHandler::NamedObjectJsonHandler(
    const CesiumJsonWriter::ExtensionWriterContext& context) noexcept
    : CesiumJsonWriter::ExtensibleObjectJsonHandler(context), _name() {}

void NamedObjectJsonHandler::reset(
    IJsonHandler* pParent,
    NamedObject* pObject) {
  CesiumJsonWriter::ExtensibleObjectJsonHandler::reset(pParent, pObject);
}

CesiumJsonWriter::IJsonHandler*
NamedObjectJsonHandler::writeObjectKeyNamedObject(
    const std::string& objectType,
    const std::string_view& str,
    NamedObject& o) {
  using namespace std::string_literals;
  if ("name"s == str)
    return property("name", this->_name, o.name);
  return this->writeObjectKeyExtensibleObject(objectType, str, o);
}
