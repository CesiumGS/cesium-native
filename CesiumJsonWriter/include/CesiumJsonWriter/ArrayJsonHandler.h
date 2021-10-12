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

namespace CesiumJsonWriter {
template <typename T, typename THandler>
class CESIUMJSONWRITER_API ArrayJsonHandler : public JsonHandler {
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

  virtual IJsonHandler* writeNull() override {
    return this->invalid("A null")->writeNull();
  }

  virtual IJsonHandler* writeBool(bool b) override {
    return this->invalid("A boolean")->writeBool(b);
  }

  virtual IJsonHandler* writeInt32(int32_t i) override {
    return this->invalid("An integer")->writeInt32(i);
  }

  virtual IJsonHandler* writeUint32(uint32_t i) override {
    return this->invalid("An integer")->writeUint32(i);
  }

  virtual IJsonHandler* writeInt64(int64_t i) override {
    return this->invalid("An integer")->writeInt64(i);
  }

  virtual IJsonHandler* writeUint64(uint64_t i) override {
    return this->invalid("An integer")->writeUint64(i);
  }

  virtual IJsonHandler* writeDouble(double d) override {
    return this->invalid("A double (floating-point)")->writeDouble(d);
  }

  virtual IJsonHandler* writeString(const std::string_view& str) override {
    return this->invalid("A string")->writeString(str);
  }

  virtual IJsonHandler* writeObjectStart() override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An object")->writeObjectStart();
    }

    assert(this->_pArray);
    T& o = this->_pArray->emplace_back();
    this->_objectHandler->reset(this, &o);
    return this->_objectHandler->writeObjectStart();
  }

  virtual IJsonHandler*
  writeObjectKey(const std::string_view& /*str*/) noexcept override {
    return nullptr;
  }

  virtual IJsonHandler* writeObjectEnd() noexcept override { return nullptr; }

  virtual IJsonHandler* writeArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->writeArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* writeArrayEnd() override { return this->parent(); }

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
class CESIUMJSONWRITER_API ArrayJsonHandler<double, DoubleJsonHandler>
    : public JsonHandler {
public:
  ArrayJsonHandler() noexcept : JsonHandler() {}

  void reset(IJsonHandler* pParent, std::vector<double>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* writeNull() override {
    return this->invalid("A null")->writeNull();
  }

  virtual IJsonHandler* writeBool(bool b) override {
    return this->invalid("A bool")->writeBool(b);
  }

  virtual IJsonHandler* writeInt32(int32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeInt32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* writeUint32(uint32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeUint32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* writeInt64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeInt64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* writeUint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeUint64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* writeDouble(double d) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeDouble(d);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(d);
    return this;
  }

  virtual IJsonHandler* writeString(const std::string_view& str) override {
    return this->invalid("A string")->writeString(str);
  }

  virtual IJsonHandler* writeObjectStart() override {
    return this->invalid("An object")->writeObjectStart();
  }

  virtual IJsonHandler* writeArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->writeArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* writeArrayEnd() override { return this->parent(); }

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
class CESIUMJSONWRITER_API ArrayJsonHandler<T, IntegerJsonHandler<T>>
    : public JsonHandler {
public:
  ArrayJsonHandler() noexcept : JsonHandler() {}

  void reset(IJsonHandler* pParent, std::vector<T>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* writeNull() override {
    return this->invalid("A null")->writeNull();
  }

  virtual IJsonHandler* writeBool(bool b) override {
    return this->invalid("A null")->writeBool(b);
  }

  virtual IJsonHandler* writeInt32(int32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeInt32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* writeUint32(uint32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeUint32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* writeInt64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeInt64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* writeUint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->writeUint64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* writeDouble(double d) override {
    return this->invalid("A double (floating-point)")->writeDouble(d);
  }

  virtual IJsonHandler* writeString(const std::string_view& str) override {
    return this->invalid("A string")->writeString(str);
  }

  virtual IJsonHandler* writeObjectStart() override {
    return this->invalid("An object")->writeObjectStart();
  }

  virtual IJsonHandler* writeArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->writeArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* writeArrayEnd() override { return this->parent(); }

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
class CESIUMJSONWRITER_API ArrayJsonHandler<std::string, StringJsonHandler>
    : public JsonHandler {
public:
  ArrayJsonHandler() noexcept : JsonHandler() {}

  void reset(IJsonHandler* pParent, std::vector<std::string>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonHandler* writeNull() override {
    return this->invalid("A null")->writeNull();
  }

  virtual IJsonHandler* writeBool(bool b) override {
    return this->invalid("A bool")->writeBool(b);
  }

  virtual IJsonHandler* writeInt32(int32_t i) override {
    return this->invalid("An integer")->writeInt32(i);
  }

  virtual IJsonHandler* writeUint32(uint32_t i) override {
    return this->invalid("An integer")->writeUint32(i);
  }

  virtual IJsonHandler* writeInt64(int64_t i) override {
    return this->invalid("An integer")->writeInt64(i);
  }

  virtual IJsonHandler* writeUint64(uint64_t i) override {
    return this->invalid("An integer")->writeUint64(i);
  }

  virtual IJsonHandler* writeDouble(double d) override {
    return this->invalid("A double (floating-point)")->writeDouble(d);
  }

  virtual IJsonHandler* writeObjectStart() override {
    return this->invalid("An object")->writeObjectStart();
  }

  virtual IJsonHandler* writeArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->writeArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonHandler* writeArrayEnd() override { return this->parent(); }

  virtual IJsonHandler* writeString(const std::string_view& str) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("A string")->writeString(str);
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
} // namespace CesiumJsonWriter
