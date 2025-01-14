#pragma once

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <optional>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler for handling JSON objects.
 */
class CESIUMJSONREADER_API ObjectJsonHandler : public JsonHandler {
public:
  ObjectJsonHandler() noexcept : JsonHandler() {}

  /** @copydoc IJsonHandler::readObjectStart */
  virtual IJsonHandler* readObjectStart() override /* final */;
  /** @copydoc IJsonHandler::readObjectEnd */
  virtual IJsonHandler* readObjectEnd() override /* final */;

protected:
  /**
   * @brief Called when \ref readObjectStart is called when the depth of the
   * \ref ObjectJsonHandler is greater than 0.
   */
  virtual IJsonHandler* StartSubObject() noexcept;
  /**
   * @brief Called when \ref readObjectEnd is called when the depth of the
   * \ref ObjectJsonHandler is greater than 0.
   */
  virtual IJsonHandler* EndSubObject() noexcept;

  /**
   * @brief Called from \ref IJsonHandler::readObjectKey to read a property into
   * an object.
   *
   * @param currentKey The key that's currently being read.
   * @param accessor An \ref IJsonHandler that can read the type of this
   * property.
   * @param value A reference to the location to store the read value.
   * @returns The `accessor` \ref IJsonHandler that will read the next token
   * into `value`.
   */
  template <typename TAccessor, typename TProperty>
  IJsonHandler*
  property(const char* currentKey, TAccessor& accessor, TProperty& value) {
    this->_currentKey = currentKey;

    if constexpr (isOptional<TProperty>::value) {
      value.emplace();
      accessor.reset(this, &value.value());
    } else if constexpr (isIntrusivePointer<TProperty>::value) {
      value.emplace();
      accessor.reset(this, value.get());
    } else {
      accessor.reset(this, &value);
    }

    return &accessor;
  }

  /**
   * @brief Obtains the most recent key handled by this JsonHandler.
   */
  const char* getCurrentKey() const noexcept;

  /** @copydoc IJsonHandler::reportWarning */
  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

protected:
  /**
   * @brief Sets the most recent key handled by this JsonHandler.
   */
  void setCurrentKey(const char* key) noexcept;

private:
  template <typename T> struct isOptional {
    static constexpr bool value = false;
  };

  template <typename T> struct isOptional<std::optional<T>> {
    static constexpr bool value = true;
  };

  template <typename T> struct isIntrusivePointer {
    static constexpr bool value = false;
  };

  template <typename T>
  struct isIntrusivePointer<CesiumUtility::IntrusivePointer<T>> {
    static constexpr bool value = true;
  };

  int32_t _depth = 0;
  const char* _currentKey = nullptr;
};
} // namespace CesiumJsonReader
