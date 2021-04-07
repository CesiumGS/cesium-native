#include "NamedObjectJsonHandler.h"
#include "CesiumGltf/NamedObject.h"
#include <ExtensibleObjectJsonHandler.h>
#include <string>

using namespace CesiumGltf;

NamedObjectJsonHandler::NamedObjectJsonHandler(
    const ReadModelOptions& options) noexcept
    : ExtensibleObjectJsonHandler(options), _name(options) {}

void NamedObjectJsonHandler::reset(
    IJsonHandler* pParent,
    NamedObject* pObject) {
  ExtensibleObjectJsonHandler::reset(pParent, pObject);
}

IJsonHandler* NamedObjectJsonHandler::NamedObjectKey(
    const std::string& objectType,
    const std::string_view& str,
    NamedObject& o) {
  using namespace std::string_literals;
  if ("name"s == str)
    return property("name", this->_name, o.name);
  return this->ExtensibleObjectKey(objectType, str, o);
}
