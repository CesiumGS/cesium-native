#pragma once

#include "CesiumUtility/Assert.h"
#include "JsonHandler.h"
#include "Library.h"

#include <cmath>

namespace CesiumJsonReader {
template <typename T>
class CESIUMJSONREADER_API IntegerJsonHandler : public JsonHandler {
public:
  IntegerJsonHandler() noexcept : JsonHandler() {}

  void reset(IJsonHandler* pParent, T* pInteger) {
    JsonHandler::reset(pParent);
    this->_pInteger = pInteger;
  }

  T* getObject() { return this->_pInteger; }

  virtual IJsonHandler* readInt32(int32_t i) override {
    CESIUM_ASSERT(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* readUint32(uint32_t i) override {
    CESIUM_ASSERT(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* readInt64(int64_t i) override {
    CESIUM_ASSERT(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* readUint64(uint64_t i) override {
    CESIUM_ASSERT(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* readDouble(double d) override {
    CESIUM_ASSERT(this->_pInteger);
    double intPart;
    double fractPart = std::modf(d, &intPart);
    if (fractPart != 0) {
      return JsonHandler::readDouble(d);
    }
    *this->_pInteger = static_cast<T>(intPart);
    return this->parent();
  }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context) override {
    context.push_back("(expecting an integer)");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  T* _pInteger = nullptr;
};
} // namespace CesiumJsonReader
