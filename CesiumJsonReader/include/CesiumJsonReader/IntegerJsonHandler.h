#pragma once

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>
#include <CesiumUtility/Assert.h>

#include <cmath>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler for reading integer values.
 */
template <typename T>
class CESIUMJSONREADER_API IntegerJsonHandler : public JsonHandler {
public:
  IntegerJsonHandler() noexcept : JsonHandler() {}

  /**
   * @brief Resets the parent \ref IJsonHandler of this instance and sets the
   * pointer to its destination integer value.
   */
  void reset(IJsonHandler* pParent, T* pInteger) {
    JsonHandler::reset(pParent);
    this->_pInteger = pInteger;
  }

  /**
   * @brief Obtains the integer pointer set on this handler by \ref reset.
   */
  T* getObject() { return this->_pInteger; }

  /** @copydoc IJsonHandler::readInt32 */
  virtual IJsonHandler* readInt32(int32_t i) override {
    CESIUM_ASSERT(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  /** @copydoc IJsonHandler::readUint32 */
  virtual IJsonHandler* readUint32(uint32_t i) override {
    CESIUM_ASSERT(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  /** @copydoc IJsonHandler::readInt64 */
  virtual IJsonHandler* readInt64(int64_t i) override {
    CESIUM_ASSERT(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  /** @copydoc IJsonHandler::readUint64 */
  virtual IJsonHandler* readUint64(uint64_t i) override {
    CESIUM_ASSERT(this->_pInteger);
    *this->_pInteger = static_cast<T>(i);
    return this->parent();
  }
  /** @copydoc IJsonHandler::readDouble */
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

  /** @copydoc IJsonHandler::reportWarning */
  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context) override {
    context.emplace_back("(expecting an integer)");
    this->parent()->reportWarning(warning, std::move(context));
  }

private:
  T* _pInteger = nullptr;
};
} // namespace CesiumJsonReader
