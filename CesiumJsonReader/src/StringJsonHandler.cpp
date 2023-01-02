#include "CesiumJsonReader/StringJsonHandler.h"

#include "CesiumJsonReader/JsonHandler.h"

namespace CesiumJsonReader {
StringJsonHandler::StringJsonHandler() noexcept : JsonHandler() {}

void StringJsonHandler::reset(IJsonHandler* pParent, std::string* pString) {
  JsonHandler::reset(pParent);
  this->_pString = pString;
}

std::string* StringJsonHandler::getObject() noexcept { return this->_pString; }

IJsonHandler* StringJsonHandler::readString(const std::string_view& str) {
  *this->_pString = str;
  return this->parent();
}
} // namespace CesiumJsonReader
