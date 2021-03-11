#include "StringJsonHandler.h"
#include <JsonHandler.h>

using namespace CesiumGltf;
StringJsonHandler::StringJsonHandler(const ReadModelOptions& options) noexcept : JsonHandler(options) {
}

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
