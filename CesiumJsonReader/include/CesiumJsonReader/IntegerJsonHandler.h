#pragma once

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>
#include <CesiumUtility/Assert.h>

#include <cmath>
#include <limits>

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
    // Reject values that are not representable in T before casting, since
    // that cast would be undefined behavior. The comparison must not use
    // static_cast<double>(max()): for 64-bit types that rounds up to 2^63
    // (or 2^64), which would incorrectly admit the first out-of-range
    // value. Instead compare against the exclusive upper bound 2^digits,
    // which is always exactly representable as a double. The lower bound
    // is zero or a negated power of two and is likewise exact.
    constexpr double upperBound =
        2.0 * static_cast<double>(std::numeric_limits<T>::max() / 2 + 1);
    if (intPart < static_cast<double>(std::numeric_limits<T>::lowest()) ||
        intPart >= upperBound) {
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
