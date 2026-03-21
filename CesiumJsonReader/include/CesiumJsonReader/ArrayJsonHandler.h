#pragma once

#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>
#include <CesiumJsonReader/StringJsonHandler.h>
#include <CesiumUtility/Assert.h>

#include <functional>
#include <memory>
#include <vector>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler for reading a JSON array into an `std::vector`.
 *
 * @tparam T The element type of the destination vector.
 * @tparam THandler The \ref IJsonHandler to handle each element.
 */
template <typename T, typename THandler>
class CESIUMJSONREADER_API ArrayJsonHandler : public JsonHandler {
public:
  /** @brief The destination type. */
  using ValueType = std::vector<T>;

  /**
   * @brief Creates a new \ref ArrayJsonHandler.
   *
   * @param args The arguments that will be passed to the constructor of
   * THandler when it's instantiated.
   */
  template <typename... Ts>
  ArrayJsonHandler(Ts&&... args) noexcept
      : JsonHandler(),
        _handlerFactory(
            std::bind(handlerFactory<Ts...>, std::forward<Ts>(args)...)),
        _objectHandler() {}

  /**
   * @brief Resets the parent and destination array of this \ref
   * ArrayJsonHandler.
   */
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

    CESIUM_ASSERT(this->_pArray);
    T& o = this->_pArray->emplace_back();
    this->_objectHandler->reset(this, &o);
    return this->_objectHandler->readObjectStart();
  }

  virtual IJsonHandler* readObjectKey(
      [[maybe_unused]] const std::string_view& str) noexcept override {
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

/**
 * @brief Special case of \ref ArrayJsonHandler for handling arrays of double
 * values. This will read every scalar value as a double, regardless of whether
 * it's floating point or not. Attempting to read other values will cause a
 * warning.
 */
template <>
class CESIUMJSONREADER_API ArrayJsonHandler<double, DoubleJsonHandler>
    : public JsonHandler {
public:
  /** @brief The destination type. */
  using ValueType = std::vector<double>;

  ArrayJsonHandler() noexcept : JsonHandler() {}

  /**
   * @brief Resets the parent and destination array of this \ref
   * ArrayJsonHandler.
   */
  void reset(IJsonHandler* pParent, std::vector<double>* pArray) {
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
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt32(i);
    }

    CESIUM_ASSERT(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* readUint32(uint32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint32(i);
    }

    CESIUM_ASSERT(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* readInt64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt64(i);
    }

    CESIUM_ASSERT(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* readUint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint64(i);
    }

    CESIUM_ASSERT(this->_pArray);
    this->_pArray->emplace_back(static_cast<double>(i));
    return this;
  }

  virtual IJsonHandler* readDouble(double d) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readDouble(d);
    }

    CESIUM_ASSERT(this->_pArray);
    this->_pArray->emplace_back(d);
    return this;
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

/**
 * @brief Special case of \ref ArrayJsonHandler for handling arrays of integer
 * values. Attempting to read other values will cause a warning.
 */
template <typename T>
class CESIUMJSONREADER_API ArrayJsonHandler<T, IntegerJsonHandler<T>>
    : public JsonHandler {
public:
  /** @brief The destination type. */
  using ValueType = std::vector<T>;

  ArrayJsonHandler() noexcept : JsonHandler() {}

  /**
   * @brief Resets the parent and destination array of this \ref
   * ArrayJsonHandler.
   */
  void reset(IJsonHandler* pParent, std::vector<T>* pArray) {
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
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt32(i);
    }

    CESIUM_ASSERT(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* readUint32(uint32_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint32(i);
    }

    CESIUM_ASSERT(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* readInt64(int64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readInt64(i);
    }

    CESIUM_ASSERT(this->_pArray);
    this->_pArray->emplace_back(static_cast<T>(i));
    return this;
  }

  virtual IJsonHandler* readUint64(uint64_t i) override {
    if (!this->_arrayIsOpen) {
      return this->invalid("An integer")->readUint64(i);
    }

    CESIUM_ASSERT(this->_pArray);
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

/**
 * @brief Special case of \ref ArrayJsonHandler for reading arrays of string
 * values. Attempting to read other values will cause a warning.
 */
template <>
class CESIUMJSONREADER_API ArrayJsonHandler<std::string, StringJsonHandler>
    : public JsonHandler {
public:
  /** @brief The destination type. */
  using ValueType = std::vector<std::string>;

  ArrayJsonHandler() noexcept : JsonHandler() {}

  /**
   * @brief Resets the parent and destination array of this \ref
   * ArrayJsonHandler.
   */
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

    CESIUM_ASSERT(this->_pArray);
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

/**
 * @brief Special case of \ref ArrayJsonHandler for reading arrays of arrays.
 * Attempting to read other values will cause a warning.
 *
 * @tparam T The inner type of the nested array.
 * @tparam THandler The handler of the inner type of the array.
 */
template <typename T, typename THandler>
class CESIUMJSONREADER_API
    ArrayJsonHandler<std::vector<T>, ArrayJsonHandler<T, THandler>>
    : public JsonHandler {
public:
  /** @brief The destination type. */
  using ValueType = std::vector<T>;

  /**
   * @brief Creates a new \ref ArrayJsonHandler.
   *
   * @param args The arguments passed to the constructor of THandler when it's
   * instantiated.
   */
  template <typename... Ts>
  ArrayJsonHandler(Ts&&... args) noexcept
      : JsonHandler(),
        _handlerFactory(
            std::bind(handlerFactory<Ts...>, std::forward<Ts>(args)...)),
        _elementHandler() {}

  /**
   * @brief Resets the parent and destination array of this \ref
   * ArrayJsonHandler.
   */
  void reset(IJsonHandler* pParent, std::vector<std::vector<T>>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
    this->_elementHandler.reset(this->_handlerFactory());
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

  virtual IJsonHandler* readString(const std::string_view& str) override {
    return this->invalid("A string")->readString(str);
  }

  virtual IJsonHandler* readObjectStart() override {
    return this->invalid("An object")->readObjectStart();
  }

  virtual IJsonHandler* readArrayStart() override {
    if (this->_arrayIsOpen) {
      CESIUM_ASSERT(this->_pArray);
      std::vector<T>& o = this->_pArray->emplace_back();
      this->_elementHandler->reset(this, &o);
      return this->_elementHandler->readArrayStart();
    } else {
      this->_arrayIsOpen = true;
      this->_pArray->clear();
      return this;
    }
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
          type + " value is not allowed in the array of arrays and has been "
                 "replaced with a default value.");
      this->_pArray->emplace_back();
      return this->ignoreAndContinue();
    } else {
      this->reportWarning(type + " is not allowed and has been ignored.");
      return this->ignoreAndReturnToParent();
    }
  }

  template <typename... Ts>
  static ArrayJsonHandler<T, THandler>* handlerFactory(Ts&&... args) {
    return new ArrayJsonHandler<T, THandler>(std::forward<Ts>(args)...);
  }

  std::vector<std::vector<T>>* _pArray = nullptr;
  bool _arrayIsOpen = false;

  std::function<ArrayJsonHandler<T, THandler>*()> _handlerFactory;
  std::unique_ptr<ArrayJsonHandler<T, THandler>> _elementHandler;
};

} // namespace CesiumJsonReader
