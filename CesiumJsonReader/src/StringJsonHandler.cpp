#include "CesiumJsonReader/StringJsonHandler.h"
#include "CesiumJsonReader/JsonReader.h"

using namespace CesiumJsonReader;

StringJsonHandler::StringJsonHandler() noexcept : JsonReader() {}

void StringJsonHandler::reset(IJsonReader* pParent, std::string* pString) {
  JsonReader::reset(pParent);
  this->_pString = pString;
}

std::string* StringJsonHandler::getObject() { return this->_pString; }

IJsonReader* StringJsonHandler::readString(const std::string_view& str) {
  *this->_pString = str;
  return this->parent();
}
