#include "BoolJsonHandler.h"
#include "CesiumGltf/JsonReader.h"
#include <cassert>

using namespace CesiumGltf;
BoolJsonHandler::BoolJsonHandler(const ReaderContext& context) noexcept
    : JsonReader(context) {}

void BoolJsonHandler::reset(IJsonReader* pParent, bool* pBool) {
  JsonReader::reset(pParent);
  this->_pBool = pBool;
}

IJsonReader* BoolJsonHandler::readBool(bool b) {
  assert(this->_pBool);
  *this->_pBool = b;
  return this->parent();
}
