#include "CesiumJsonReader/BoolJsonHandler.h"
#include "CesiumJsonReader/JsonReader.h"
#include <cassert>

using namespace CesiumJsonReader;

BoolJsonHandler::BoolJsonHandler() noexcept : JsonReader() {}

void BoolJsonHandler::reset(IJsonReader* pParent, bool* pBool) {
  JsonReader::reset(pParent);
  this->_pBool = pBool;
}

IJsonReader* BoolJsonHandler::readBool(bool b) {
  assert(this->_pBool);
  *this->_pBool = b;
  return this->parent();
}
