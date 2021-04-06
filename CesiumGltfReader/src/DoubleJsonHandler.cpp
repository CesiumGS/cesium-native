#include "DoubleJsonHandler.h"

using namespace CesiumGltf;

DoubleJsonHandler::DoubleJsonHandler(const ReadModelOptions& options) noexcept
    : JsonHandler(options) {}

void DoubleJsonHandler::reset(IJsonHandler* pParent, double* pDouble) {
  JsonHandler::reset(pParent);
  this->_pDouble = pDouble;
}

IJsonHandler* DoubleJsonHandler::readInt32(int i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::readUint32(unsigned i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::readInt64(int64_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::readUint64(uint64_t i) {
  assert(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::readDouble(double d) {
  assert(this->_pDouble);
  *this->_pDouble = d;
  return this->parent();
}
