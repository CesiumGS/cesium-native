#include "AttributeTypeJsonHandler.h"
#include <string>

using namespace CesiumGltf;

void AttributeTypeJsonHandler::reset(JsonHandler* pParent, AttributeType* pEnum) {
    JsonHandler::reset(pParent);
    this->_pEnum = pEnum;
}

JsonHandler* AttributeTypeJsonHandler::String(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    if ("SCALAR"s == str) *this->_pEnum = AttributeType::SCALAR;
    else if ("VEC2"s == str) *this->_pEnum = AttributeType::VEC2;
    else if ("VEC3"s == str) *this->_pEnum = AttributeType::VEC3;
    else if ("VEC4"s == str) *this->_pEnum = AttributeType::VEC4;
    else if ("MAT2"s == str) *this->_pEnum = AttributeType::MAT2;
    else if ("MAT3"s == str) *this->_pEnum = AttributeType::MAT3;
    else if ("MAT4"s == str) *this->_pEnum = AttributeType::MAT4;
    else return nullptr;

    return this->parent();
}
