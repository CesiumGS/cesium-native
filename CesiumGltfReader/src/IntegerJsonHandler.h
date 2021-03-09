#pragma once

#include "JsonHandler.h"
#include <cassert>

namespace CesiumGltf {
template <typename T> class IntegerJsonHandler : public JsonHandler {
public:
  void reset(IJsonHandler* pParent, T* pInteger) {
    JsonHandler::reset(pParent);
    this->_pInteger = pInteger;
  }

  T* getObject() { return this->_pInteger; }

  virtual IJsonHandler* Int(int i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* Uint(unsigned i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* Int64(int64_t i) override {
    assert(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  virtual IJsonHandler* Uint64(uint64_t i) override {
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
} // namespace CesiumGltf
