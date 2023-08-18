#pragma once

#include "DoubleJsonHandler.h"
#include "IntegerJsonHandler.h"
#include "JsonHandler.h"
#include "Library.h"
#include "StringJsonHandler.h"

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace CesiumJsonReader {
template <typename T, typename THandler>
class CESIUMJSONREADER_API ArrayJsonHandler : public JsonHandler {
public:
  template <typename... Ts>
  ArrayJsonHandler(Ts&&... args) noexcept
      : JsonHandler(),
        _handlerFactory(
            std::bind(handlerFactory<Ts...>, std::forward<Ts>(args)...)),
        _objectHandler() {}

  void reset(IJsonHandler* pParent, std::vector<T>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
    this->_objectHandler.reset(this->_handlerFactory());
  }

  virtual IJsonHandler* readNull() override {
    return this->invalid("A null")->readNull();
  }

  virtual IJsonHandler* readBool(bool b) override {
    return this->invalid("A boolean")->readBool(b);
  }

  virtual IJsonHandler* readInt32(int32_t i) override {
    return this->invalid("An integer")->readInt32(i);
  }

  virtual IJsonHandler* readUint32(uint32_t i) override {
    return this->invalid("An integer")->readUint32(i);
  }

  virtual IJsonHandler* readInt64(int64_t i) override {
    return this->invalid("An integer")->readInt64(i);
  }

  virtual IJsonHandler* readUint64(uint64_t i) override {
    return this->invalid("An integer")->readUint64(i);
  }

  virtual IJsonHandler* readDouble(double d) override {
    return this->invalid("A double (floating-point)")->readDouble(d);
  }

  virtual IJsonHandler* readString(const std::string_view& str) override {
    return this->invalid("A string")->readString(str);
  }

  virtual IJsonHandler* readObjectStart() override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An object")->readObjectStart();
    }

    assert(this->_pArray);
    T& o = this->_pArray->emplace_back();
    this->_objectHandler->reset(this, &o);
    return this->_objectHandler->readObjectStart();
  }

  virtual IJsonHandler*
  readObjectKey(const std::string_view& /*str*/) noexcept override {
    return nullptr;
  }

  virtual IJsonHandler* readObjectEnd() noexcept override { return nullptr; }

  virtual IJsonHandler* readArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->readArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* readArrayEnd() override { return this->parent(); }

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

  template <typename... Ts> static THandler* handlerFactory(Ts&&... args) {
    return new THandler(std::forward<Ts>(args)...);
  }

  std::vector<T>* _pArray = nullptr;
  bool _arrayIsOpen = false;

  std::function<THandler*()> _handlerFactory;
  std::unique_ptr<THandler> _objectHandler;
};

template <>
class CESIUMJSONREADER_API ArrayJsonHandler<double, DoubleJsonHandler>
    : public JsonHandler {
public:
  ArrayJsonHandler() noexcept : JsonHandler() {}

  void reset(IJsonHandler* pParent, std::vector<double>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_pNext = nullptr;
    this->_pEnd = nullptr;
    this->_arrayIsOpen = false;
  }

  template <size_t N>
  void reset(IJsonHandler* pParent, std::array<double, N>* pArray) {
    JsonHandler::reset(pParent);
    this->_pNext = pArray->data();
    this->_pEnd = this->_pNext + pArray->size();
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* readNull() override {
    return this->invalid("A null")->readNull();
  }

  virtual IJsonHandler* readBool(bool b) override {
    return this->invalid("A bool")->readBool(b);
  }

  virtual IJsonHandler* readInt32(int32_t i) override {
    return this->storeNext("An integer", static_cast<double>(i));
  }

  virtual IJsonHandler* readUint32(uint32_t i) override {
    return this->storeNext("An integer", static_cast<double>(i));
  }

  virtual IJsonHandler* readInt64(int64_t i) override {
    return this->storeNext("An integer", static_cast<double>(i));
  }

  virtual IJsonHandler* readUint64(uint64_t i) override {
    return this->storeNext("An integer", static_cast<double>(i));
  }

  virtual IJsonHandler* readDouble(double d) override {
    return this->storeNext("A double", d);
  }

  virtual IJsonHandler* readString(const std::string_view& str) override {
    return this->invalid("A string")->readString(str);
  }

  virtual IJsonHandler* readObjectStart() override {
    return this->invalid("An object")->readObjectStart();
  }

  virtual IJsonHandler* readArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->readArrayStart();
    }

    this->_arrayIsOpen = true;
    if (this->_pArray != nullptr)
      this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* readArrayEnd() override { return this->parent(); }

  IJsonHandler* storeNext(const char* typeRead, double value) {
    if (!this->_arrayIsOpen) {
      this->reportWarning(
          std::string(typeRead) + " is not allowed and has been ignored.");
      return this->ignoreAndReturnToParent();
    }

    if (this->_pArray) {
      this->_pArray->emplace_back(value);
      return this;
    } else if (this->_pNext != this->_pEnd) {
      *this->_pNext = value;
      ++this->_pNext;
      return this;
    } else if (this->_pNext != nullptr) {
      this->reportWarning(
          "Fixed-size array has too many items. The extras will be ignored.");
      return this->ignoreAndContinue();
    } else {
      // This is a developer error and shouldn't happen.
      // Either _pArray or _pNext/_pEnd must be set.
      assert(false);
      return this;
    }
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
          type + " value is not allowed in the double array and has been "
                 "replaced with a default value.");
      return this->storeNext(type.c_str(), 0.0);
    } else {
      this->reportWarning(type + " is not allowed and has been ignored.");
      return this->ignoreAndReturnToParent();
    }
  }

  std::vector<double>* _pArray = nullptr;
  bool _arrayIsOpen = false;
  double* _pNext = nullptr;
  double* _pEnd = nullptr;
};

template <typename T>
class CESIUMJSONREADER_API ArrayJsonHandler<T, IntegerJsonHandler<T>>
    : public JsonHandler {
public:
  ArrayJsonHandler() noexcept : JsonHandler() {}

  void reset(IJsonHandler* pParent, std::vector<T>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* readNull() override {
    return this->invalid("A null")->readNull();
  }

  virtual IJsonHandler* readBool(bool b) override {
    return this->invalid("A null")->readBool(b);
  }

  virtual IJsonHandler* readInt32(int32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* readUint32(uint32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* readInt64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* readUint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* readDouble(double d) override {
    return this->invalid("A double (floating-point)")->readDouble(d);
  }

  virtual IJsonHandler* readString(const std::string_view& str) override {
    return this->invalid("A string")->readString(str);
  }

  virtual IJsonHandler* readObjectStart() override {
    return this->invalid("An object")->readObjectStart();
  }

  virtual IJsonHandler* readArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->readArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* readArrayEnd() override { return this->parent(); }

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
class CESIUMJSONREADER_API ArrayJsonHandler<std::string, StringJsonHandler>
    : public JsonHandler {
public:
  ArrayJsonHandler() noexcept : JsonHandler() {}

  void reset(IJsonHandler* pParent, std::vector<std::string>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* readNull() override {
    return this->invalid("A null")->readNull();
  }

  virtual IJsonHandler* readBool(bool b) override {
    return this->invalid("A bool")->readBool(b);
  }

  virtual IJsonHandler* readInt32(int32_t i) override {
    return this->invalid("An integer")->readInt32(i);
  }

  virtual IJsonHandler* readUint32(uint32_t i) override {
    return this->invalid("An integer")->readUint32(i);
  }

  virtual IJsonHandler* readInt64(int64_t i) override {
    return this->invalid("An integer")->readInt64(i);
  }

  virtual IJsonHandler* readUint64(uint64_t i) override {
    return this->invalid("An integer")->readUint64(i);
  }

  virtual IJsonHandler* readDouble(double d) override {
    return this->invalid("A double (floating-point)")->readDouble(d);
  }

  virtual IJsonHandler* readObjectStart() override {
    return this->invalid("An object")->readObjectStart();
  }

  virtual IJsonHandler* readArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->readArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* readArrayEnd() override { return this->parent(); }

  virtual IJsonHandler* readString(const std::string_view& str) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("A string")->readString(str);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(str);
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
} // namespace CesiumJsonReader
