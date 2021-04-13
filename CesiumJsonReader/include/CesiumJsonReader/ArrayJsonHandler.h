#pragma once

#include "CesiumJsonReader/JsonReader.h"
#include "DoubleJsonHandler.h"
#include "IntegerJsonHandler.h"
#include "StringJsonHandler.h"
#include <cassert>
#include <vector>

namespace CesiumGltf {
template <typename T, typename THandler>
class ArrayJsonHandler : public JsonReader {
public:
  template <typename... Ts>
  ArrayJsonHandler(Ts&&... args) noexcept
      : JsonReader(), _objectHandler(std::forward<Ts>(args)...) {}

  void reset(IJsonReader* pParent, std::vector<T>* pArray) {
    JsonReader::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonReader* readNull() override {
    return this->invalid("A null")->readNull();
  }

  virtual IJsonReader* readBool(bool b) override {
    return this->invalid("A boolean")->readBool(b);
  }

  virtual IJsonReader* readInt32(int32_t i) override {
    return this->invalid("An integer")->readInt32(i);
  }

  virtual IJsonReader* readUint32(uint32_t i) override {
    return this->invalid("An integer")->readUint32(i);
  }

  virtual IJsonReader* readInt64(int64_t i) override {
    return this->invalid("An integer")->readInt64(i);
  }

  virtual IJsonReader* readUint64(uint64_t i) override {
    return this->invalid("An integer")->readUint64(i);
  }

  virtual IJsonReader* readDouble(double d) override {
    return this->invalid("A double (floating-point)")->readDouble(d);
  }

  virtual IJsonReader* readString(const std::string_view& str) override {
    return this->invalid("A string")->readString(str);
  }

  virtual IJsonReader* readObjectStart() override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An object")->readObjectStart();
    }

    assert(this->_pArray);
    T& o = this->_pArray->emplace_back();
    this->_objectHandler.reset(this, &o);
    return this->_objectHandler.readObjectStart();
  }

  virtual IJsonReader* readObjectKey(const std::string_view& /*str*/) override {
    return nullptr;
  }

  virtual IJsonReader* readObjectEnd() override { return nullptr; }

  virtual IJsonReader* readArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->readArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonReader* readArrayEnd() override { return this->parent(); }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    context.push_back(
        std::string("[") + std::to_string(this->_pArray->size()) + "]");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  IJsonReader* invalid(const std::string& type) {
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
class ArrayJsonHandler<double, DoubleJsonHandler> : public JsonReader {
public:
  ArrayJsonHandler() : JsonReader() {}

  void reset(IJsonReader* pParent, std::vector<double>* pArray) {
    JsonReader::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonReader* readNull() override {
    return this->invalid("A null")->readNull();
  }

  virtual IJsonReader* readBool(bool b) override {
    return this->invalid("A bool")->readBool(b);
  }

  virtual IJsonReader* readInt32(int32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonReader* readUint32(uint32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonReader* readInt64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonReader* readUint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonReader* readDouble(double d) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readDouble(d);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(d);
    return this;
  }

  virtual IJsonReader* readString(const std::string_view& str) override {
    return this->invalid("A string")->readString(str);
  }

  virtual IJsonReader* readObjectStart() override {
    return this->invalid("An object")->readObjectStart();
  }

  virtual IJsonReader* readArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->readArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonReader* readArrayEnd() override { return this->parent(); }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    context.push_back(
        std::string("[") + std::to_string(this->_pArray->size()) + "]");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  IJsonReader* invalid(const std::string& type) {
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
class ArrayJsonHandler<T, IntegerJsonHandler<T>> : public JsonReader {
public:
  ArrayJsonHandler() : JsonReader() {}

  void reset(IJsonReader* pParent, std::vector<T>* pArray) {
    JsonReader::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonReader* readNull() override {
    return this->invalid("A null")->readNull();
  }

  virtual IJsonReader* readBool(bool b) override {
    return this->invalid("A null")->readBool(b);
  }

  virtual IJsonReader* readInt32(int32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonReader* readUint32(uint32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint32(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonReader* readInt64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonReader* readUint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint64(i);
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonReader* readDouble(double d) override {
    return this->invalid("A double (floating-point)")->readDouble(d);
  }

  virtual IJsonReader* readString(const std::string_view& str) override {
    return this->invalid("A string")->readString(str);
  }

  virtual IJsonReader* readObjectStart() override {
    return this->invalid("An object")->readObjectStart();
  }

  virtual IJsonReader* readArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->readArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonReader* readArrayEnd() override { return this->parent(); }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    context.push_back(
        std::string("[") + std::to_string(this->_pArray->size()) + "]");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  IJsonReader* invalid(const std::string& type) {
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
class ArrayJsonHandler<std::string, StringJsonHandler> : public JsonReader {
public:
  ArrayJsonHandler() : JsonReader() {}

  void reset(IJsonReader* pParent, std::vector<std::string>* pArray) {
    JsonReader::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
  }

  virtual IJsonReader* readNull() override {
    return this->invalid("A null")->readNull();
  }

  virtual IJsonReader* readBool(bool b) override {
    return this->invalid("A bool")->readBool(b);
  }

  virtual IJsonReader* readInt32(int32_t i) override {
    return this->invalid("An integer")->readInt32(i);
  }

  virtual IJsonReader* readUint32(uint32_t i) override {
    return this->invalid("An integer")->readUint32(i);
  }

  virtual IJsonReader* readInt64(int64_t i) override {
    return this->invalid("An integer")->readInt64(i);
  }

  virtual IJsonReader* readUint64(uint64_t i) override {
    return this->invalid("An integer")->readUint64(i);
  }

  virtual IJsonReader* readDouble(double d) override {
    return this->invalid("A double (floating-point)")->readDouble(d);
  }

  virtual IJsonReader* readObjectStart() override {
    return this->invalid("An object")->readObjectStart();
  }

  virtual IJsonReader* readArrayStart() override {
    if (this->_arrayIsOpen) {
      return this->invalid("An array")->readArrayStart();
    }

    this->_arrayIsOpen = true;
    this->_pArray->clear();
    return this;
  }

  virtual IJsonReader* readArrayEnd() override { return this->parent(); }

  virtual IJsonReader* readString(const std::string_view& str) override {
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
  IJsonReader* invalid(const std::string& type) {
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
