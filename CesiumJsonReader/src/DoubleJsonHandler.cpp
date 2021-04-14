#include "CesiumJsonReader/DoubleJsonHandler.h"

using namespace CesiumJsonReader;

DoubleJsonHandler::DoubleJsonHandler() noexcept : JsonReader() {}

void DoubleJsonHandler::reset(IJsonReader* pParent, double* pDouble) {
  JsonReader::reset(pParent);
  this->_pDouble = pDouble;
}

IJsonReader* DoubleJsonHandler::readInt32(int32_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonReader* DoubleJsonHandler::readUint32(uint32_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonReader* DoubleJsonHandler::readInt64(int64_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonReader* DoubleJsonHandler::readUint64(uint64_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonReader* DoubleJsonHandler::readDouble(double d) {
  assert(this->_pDouble);
  *this->_pDouble = d;
  return this->parent();
}
