#include "NamedObjectJsonHandler.h"
#include "CesiumGltf/NamedObject.h"
#include <string>

using namespace CesiumGltf;

IJsonHandler* NamedObjectJsonHandler::NamedObjectKey(const char* str, NamedObject& o) {
    using namespace std::string_literals;
    if ("name"s == str) return property("name", this->_name, o.name);
    return this->ExtensibleObjectKey(str, o);
}
