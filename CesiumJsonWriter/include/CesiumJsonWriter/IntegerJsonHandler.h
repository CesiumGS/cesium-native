#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <cassert>

namespace CesiumJsonWriter {
template <typename T>
class CESIUMJSONWRITER_API IntegerJsonHandler : public JsonHandler {
public:
  IntegerJsonHandler() noexcept : JsonHandler() {}

  void reset(IJsonHandler* pParent, T* pInteger) {
    JsonHandler::reset(pParent);
    this->_pInteger = pInteger;
  }

  T* getObject() { return this->_pInteger; }

  virtual IJsonHandler* writeInt32(int32_t i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* writeUint32(uint32_t i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* writeInt64(int64_t i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* writeUint64(uint64_t i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
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
} // namespace CesiumJsonWriter
