#include "StringJsonHandler.h"
#include "CesiumGltf/JsonReader.h"

using namespace CesiumGltf;
StringJsonHandler::StringJsonHandler(const ReaderContext& context) noexcept
    : JsonReader(context) {}

void StringJsonHandler::reset(IJsonReader* pParent, std::string* pString) {
  JsonReader::reset(pParent);
  this->_pString = pString;
}

std::string* StringJsonHandler::getObject() { return this->_pString; }

IJsonReader* StringJsonHandler::readString(const std::string_view& str) {
  *this->_pString = str;
  return this->parent();
}
