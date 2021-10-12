#include "CesiumJsonWriter/DoubleJsonHandler.h"

using namespace CesiumJsonWriter;

DoubleJsonHandler::DoubleJsonHandler() noexcept : JsonHandler() {}

void DoubleJsonHandler::reset(IJsonHandler* pParent, double* pDouble) {
  JsonHandler::reset(pParent);
  this->_pDouble = pDouble;
}

IJsonHandler* DoubleJsonHandler::writeInt32(int32_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::writeUint32(uint32_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::writeInt64(int64_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::writeUint64(uint64_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::writeDouble(double d) {
  assert(this->_pDouble);
  *this->_pDouble = d;
  return this->parent();
}
