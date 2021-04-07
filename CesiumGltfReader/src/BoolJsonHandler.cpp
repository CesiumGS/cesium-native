#include "BoolJsonHandler.h"
#include "CesiumGltf/JsonReader.h"
#include <cassert>

using namespace CesiumGltf;
BoolJsonHandler::BoolJsonHandler(const JsonReaderContext& context) noexcept
    : JsonHandler(context) {}

void BoolJsonHandler::reset(IJsonHandler* pParent, bool* pBool) {
  JsonHandler::reset(pParent);
  this->_pBool = pBool;
}

IJsonHandler* BoolJsonHandler::readBool(bool b) {
  assert(this->_pBool);
  *this->_pBool = b;
  return this->parent();
}
