#include "NamedObjectJsonHandler.h"
#include "CesiumGltf/NamedObject.h"
#include <string>

using namespace CesiumGltf;

void NamedObjectJsonHandler::reset(IJsonHandler* pParent, NamedObject* pObject) {
    ExtensibleObjectJsonHandler::reset(pParent, pObject);
}

IJsonHandler* NamedObjectJsonHandler::NamedObjectKey(const char* str, NamedObject& o) {
    using namespace std::string_literals;
    if ("name"s == str) return property("name", this->_name, o.name);
    return this->ExtensibleObjectKey(str, o);
}
