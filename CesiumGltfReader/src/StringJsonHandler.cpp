#include "StringJsonHandler.h"
#include "CesiumGltf/JsonReader.h"

using namespace CesiumGltf;
StringJsonHandler::StringJsonHandler(const JsonReaderContext& context) noexcept
    : JsonHandler(context) {}

void StringJsonHandler::reset(IJsonHandler* pParent, std::string* pString) {
  JsonHandler::reset(pParent);
  this->_pString = pString;
}

std::string* StringJsonHandler::getObject() { return this->_pString; }

IJsonHandler* StringJsonHandler::readString(const std::string_view& str) {
  *this->_pString = str;
  return this->parent();
}
