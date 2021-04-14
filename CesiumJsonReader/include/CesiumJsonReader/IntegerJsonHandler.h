#pragma once

#include "CesiumJsonReader/JsonReader.h"
#include "CesiumJsonReader/Library.h"
#include <cassert>

namespace CesiumJsonReader {
template <typename T>
class CESIUMJSONREADER_API IntegerJsonHandler : public JsonReader {
public:
  IntegerJsonHandler() noexcept : JsonReader() {}

  void reset(IJsonReader* pParent, T* pInteger) {
    JsonReader::reset(pParent);
    this->_pInteger = pInteger;
  }

  T* getObject() { return this->_pInteger; }

  virtual IJsonReader* readInt32(int32_t i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonReader* readUint32(uint32_t i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonReader* readInt64(int64_t i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonReader* readUint64(uint64_t i) override {
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
} // namespace CesiumJsonReader
