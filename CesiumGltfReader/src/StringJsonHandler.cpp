#include "StringJsonHandler.h"

using namespace CesiumGltf;

void StringJsonHandler::reset(JsonHandler* pParent, std::string* pString) {
    JsonHandler::reset(pParent);
    this->_pString = pString;
}

JsonHandler* StringJsonHandler::String(const char* str, size_t length, bool /*copy*/) {
    *this->_pString = std::string(str, length);
    return this->parent();
}
