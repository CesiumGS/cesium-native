#include "CesiumJsonReader/BoolJsonHandler.h"

#include "CesiumJsonReader/JsonHandler.h"

#include <cassert>

namespace CesiumJsonReader {
BoolJsonHandler::BoolJsonHandler() noexcept : JsonHandler() {}

void BoolJsonHandler::reset(IJsonHandler* pParent, bool* pBool) {
  JsonHandler::reset(pParent);
  this->_pBool = pBool;
}

IJsonHandler* BoolJsonHandler::readBool(bool b) {
  assert(this->_pBool);
  *this->_pBool = b;
  return this->parent();
}
} // namespace CesiumJsonReader
