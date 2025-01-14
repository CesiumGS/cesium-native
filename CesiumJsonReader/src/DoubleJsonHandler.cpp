#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumUtility/Assert.h>

#include <cstdint>

namespace CesiumJsonReader {
DoubleJsonHandler::DoubleJsonHandler() noexcept : JsonHandler() {}

void DoubleJsonHandler::reset(IJsonHandler* pParent, double* pDouble) {
  JsonHandler::reset(pParent);
  this->_pDouble = pDouble;
}

IJsonHandler* DoubleJsonHandler::readInt32(int32_t i) {
  CESIUM_ASSERT(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::readUint32(uint32_t i) {
  CESIUM_ASSERT(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::readInt64(int64_t i) {
  CESIUM_ASSERT(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::readUint64(uint64_t i) {
  CESIUM_ASSERT(this->_pDouble);
  *this->_pDouble = static_cast<double>(i);
  return this->parent();
}

IJsonHandler* DoubleJsonHandler::readDouble(double d) {
  CESIUM_ASSERT(this->_pDouble);
  *this->_pDouble = d;
  return this->parent();
}
} // namespace CesiumJsonReader
