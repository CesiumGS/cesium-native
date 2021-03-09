#include "StringJsonHandler.h"

using namespace CesiumGltf;

void StringJsonHandler::reset(IJsonHandler* pParent, std::string* pString) {
  JsonHandler::reset(pParent);
  this->_pString = pString;
}

std::string* StringJsonHandler::getObject() { return this->_pString; }

IJsonHandler*
StringJsonHandler::String(const char* str, size_t length, bool /*copy*/) {
  *this->_pString = std::string(str, length);
  return this->parent();
}
