#include "CesiumJsonWriter/BoolJsonHandler.h"

#include "CesiumJsonWriter/JsonHandler.h"

#include <cassert>

using namespace CesiumJsonWriter;

BoolJsonHandler::BoolJsonHandler() noexcept : JsonHandler() {}

void BoolJsonHandler::reset(IJsonHandler* pParent, bool* pBool) {
  JsonHandler::reset(pParent);
  this->_pBool = pBool;
}

IJsonHandler* BoolJsonHandler::writeBool(bool b) {
  assert(this->_pBool);
  *this->_pBool = b;
  return this->parent();
}
