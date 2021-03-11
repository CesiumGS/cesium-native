#pragma once

#include "DoubleJsonHandler.h"
#include "IntegerJsonHandler.h"
#include "JsonHandler.h"
#include "StringJsonHandler.h"
#include <cassert>
#include <vector>

namespace CesiumGltf {
template <typename T, typename THandler>
class ArrayJsonHandler : public JsonHandler {
public:
  ArrayJsonHandler(ReadModelOptions options) noexcept : JsonHandler(options), _objectHandler(options) {}

  void reset(IJsonHandler* pParent, std::vector<T>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* Null() override {
    return this->invalid("A null")->Null();
  }

  virtual IJsonHandler* Bool(bool b) override {
    return this->invalid("A boolean")->Bool(b);
  }

  virtual IJsonHandler* Int(int i) override {
    return this->invalid("An integer")->Int(i);
  }

  virtual IJsonHandler* Uint(unsigned i) override {
    return this->invalid("An integer")->Uint(i);
  }

  virtual IJsonHandler* Int64(int64_t i) override {
    return this->invalid("An integer")->Int64(i);
  }

  virtual IJsonHandler* Uint64(uint64_t i) override {
    return this->invalid("An integer")->Uint64(i);
  }

  virtual IJsonHandler* Double(double d) override {
    return this->invalid("A double (floating-point)")->Double(d);
  }

  virtual IJsonHandler*
  String(const char* str, size_t length, bool copy) override {
    return this->invalid("A string")->String(str, length, copy);
  }

  virtual IJsonHandler* StartObject() override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An object")->StartObject();
    }

    assert(this->_pArray);
    T& o = this->_pArray->emplace_back();
    this->_objectHandler.reset(this, &o);
    return this->_objectHandler.StartObject();
  }

  virtual IJsonHandler*
  Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) override {
    return nullptr;
  }

  virtual IJsonHandler* EndObject(size_t /*memberCount*/) override {
    return nullptr;
  }

  virtual IJsonHandler* StartArray() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->StartArray();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* EndArray(size_t) override { return this->parent(); }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    context.push_back(
        std::string("[") + std::to_string(this->_pArray->size()) + "]");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  IJsonHandler* invalid(const std::string& type) {
    if (this->_arrayIsOpen) {
      this->reportWarning(
          type + " value is not allowed in the object array and has been "
                 "replaced with a default value.");
      this->_pArray->emplace_back();
      return this->ignoreAndContinue();
    } else {
      this->reportWarning(type + " is not allowed and has been ignored.");
      return this->ignoreAndReturnToParent();
    }
  }

  std::vector<T>* _pArray = nullptr;
  bool _arrayIsOpen = false;
  THandler _objectHandler;
};

template <>
class ArrayJsonHandler<double, DoubleJsonHandler> : public JsonHandler {
public:
  ArrayJsonHandler(ReadModelOptions options) : JsonHandler(options) {}

  void reset(IJsonHandler* pParent, std::vector<double>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* Null() override {
    return this->invalid("A null")->Null();
  }

  virtual IJsonHandler* Bool(bool b) override {
    return this->invalid("A bool")->Bool(b);
  }

  virtual IJsonHandler* Int(int i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Int(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* Uint(unsigned i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Uint(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* Int64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Int64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* Uint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Uint64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* Double(double d) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Double(d);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(d);
    return this;
  }

  virtual IJsonHandler*
  String(const char* str, size_t length, bool copy) override {
    return this->invalid("A string")->String(str, length, copy);
  }

  virtual IJsonHandler* StartObject() override {
    return this->invalid("An object")->StartObject();
  }

  virtual IJsonHandler* StartArray() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->StartArray();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* EndArray(size_t) override { return this->parent(); }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    context.push_back(
        std::string("[") + std::to_string(this->_pArray->size()) + "]");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  IJsonHandler* invalid(const std::string& type) {
    if (this->_arrayIsOpen) {
      this->reportWarning(
          type + " value is not allowed in the double array and has been "
                 "replaced with a default value.");
      this->_pArray->emplace_back();
      return this->ignoreAndContinue();
    } else {
      this->reportWarning(type + " is not allowed and has been ignored.");
      return this->ignoreAndReturnToParent();
    }
  }

  std::vector<double>* _pArray = nullptr;
  bool _arrayIsOpen = false;
};

template <typename T>
class ArrayJsonHandler<T, IntegerJsonHandler<T>> : public JsonHandler {
public:
  ArrayJsonHandler(ReadModelOptions options) : JsonHandler(options) {}

  void reset(IJsonHandler* pParent, std::vector<T>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* Null() override {
    return this->invalid("A null")->Null();
  }

  virtual IJsonHandler* Bool(bool b) override {
    return this->invalid("A null")->Bool(b);
  }

  virtual IJsonHandler* Int(int i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Int(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* Uint(unsigned i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Uint(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* Int64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Int64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* Uint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->Uint64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* Double(double d) override {
    return this->invalid("A double (floating-point)")->Double(d);
  }

  virtual IJsonHandler*
  String(const char* str, size_t length, bool copy) override {
    return this->invalid("A string")->String(str, length, copy);
  }

  virtual IJsonHandler* StartObject() override {
    return this->invalid("An object")->StartObject();
  }

  virtual IJsonHandler* StartArray() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->StartArray();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* EndArray(size_t) override { return this->parent(); }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    context.push_back(
        std::string("[") + std::to_string(this->_pArray->size()) + "]");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  IJsonHandler* invalid(const std::string& type) {
    if (this->_arrayIsOpen) {
      this->reportWarning(
          type + " value is not allowed in the integer array and has been "
                 "replaced with a default value.");
      this->_pArray->emplace_back();
      return this->ignoreAndContinue();
    } else {
      this->reportWarning(type + " is not allowed and has been ignored.");
      return this->ignoreAndReturnToParent();
    }
  }

  std::vector<T>* _pArray = nullptr;
  bool _arrayIsOpen = false;
};

template <>
class ArrayJsonHandler<std::string, StringJsonHandler> : public JsonHandler {
public:
  ArrayJsonHandler(ReadModelOptions options) : JsonHandler(options) {}

  void reset(IJsonHandler* pParent, std::vector<std::string>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* Null() override {
    return this->invalid("A null")->Null();
  }

  virtual IJsonHandler* Bool(bool b) override {
    return this->invalid("A bool")->Bool(b);
  }

  virtual IJsonHandler* Int(int i) override {
    return this->invalid("An integer")->Int(i);
  }

  virtual IJsonHandler* Uint(unsigned i) override {
    return this->invalid("An integer")->Uint(i);
  }

  virtual IJsonHandler* Int64(int64_t i) override {
    return this->invalid("An integer")->Int64(i);
  }

  virtual IJsonHandler* Uint64(uint64_t i) override {
    return this->invalid("An integer")->Uint64(i);
  }

  virtual IJsonHandler* Double(double d) override {
    return this->invalid("A double (floating-point)")->Double(d);
  }

  virtual IJsonHandler* StartObject() override {
    return this->invalid("An object")->StartObject();
  }

  virtual IJsonHandler* StartArray() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->StartArray();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* EndArray(size_t) override { return this->parent(); }

  virtual IJsonHandler*
  String(const char* str, size_t length, bool copy) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("A string")->String(str, length, copy);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(std::string(str, length));
    return this;
  }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    context.push_back(
        std::string("[") + std::to_string(this->_pArray->size()) + "]");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  IJsonHandler* invalid(const std::string& type) {
    if (this->_arrayIsOpen) {
      this->reportWarning(
          type + " value is not allowed in the string array and has been "
                 "replaced with a default value.");
      this->_pArray->emplace_back();
      return this->ignoreAndContinue();
    } else {
      this->reportWarning(type + " is not allowed and has been ignored.");
      return this->ignoreAndReturnToParent();
    }
  }

  std::vector<std::string>* _pArray = nullptr;
  bool _arrayIsOpen = false;
};
} // namespace CesiumGltf
