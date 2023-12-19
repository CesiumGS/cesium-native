#pragma once

#include "Library.h"

#include <gsl/narrow>

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace CesiumUtility {

struct JsonValueMissingKey : public std::runtime_error {
  JsonValueMissingKey(const std::string& key)
      : std::runtime_error(key + " is not present in Object") {}
};

struct JsonValueNotRealValue : public std::runtime_error {
  JsonValueNotRealValue()
      : std::runtime_error("this->value was not double, uint64_t or int64_t") {}
};

template <typename T, typename U>
constexpr std::optional<T> losslessNarrow(U u) noexcept {
  constexpr const bool is_different_signedness =
      (std::is_signed<T>::value != std::is_signed<U>::value);

  const T t = gsl::narrow_cast<T>(u);

  if (static_cast<U>(t) != u ||
      (is_different_signedness && ((t < T{}) != (u < U{})))) {
    return std::nullopt;
  }

  return t;
}

template <typename T, typename U>
constexpr T losslessNarrowOrDefault(U u, T defaultValue) noexcept {
  constexpr const bool is_different_signedness =
      (std::is_signed<T>::value != std::is_signed<U>::value);

  const T t = gsl::narrow_cast<T>(u);

  if (static_cast<U>(t) != u ||
      (is_different_signedness && ((t < T{}) != (u < U{})))) {
    return defaultValue;
  }

  return t;
}

/**
 * @brief A generic implementation of a value in a JSON structure.
 *
 * Instances of this class are used to represent the common `extras` field
 * of glTF elements that extend the the {@link ExtensibleObject} class.
 */
class CESIUMUTILITY_API JsonValue final {
public:
  /**
   * @brief The type to represent a `null` JSON value.
   */
  using Null = std::nullptr_t;

  /**
   * @brief The type to represent a `Bool` JSON value.
   */
  using Bool = bool;

  /**
   * @brief The type to represent a `String` JSON value.
   */
  using String = std::string;

  /**
   * @brief The type to represent an `Object` JSON value.
   */
  using Object = std::map<std::string, JsonValue>;

  /**
   * @brief The type to represent an `Array` JSON value.
   */
  using Array = std::vector<JsonValue>;

  /**
   * @brief Default constructor.
   */
  JsonValue() noexcept : value() {}

  /**
   * @brief Creates a `null` JSON value.
   */
  JsonValue(std::nullptr_t) noexcept : value(nullptr) {}

  /**
   * @brief Creates a `Number` JSON value.
   *
   * NaN and ±Infinity are represented as {@link JsonValue::Null}.
   */
  JsonValue(double v) noexcept {
    if (std::isnan(v) || std::isinf(v)) {
      value = nullptr;
    } else {
      value = v;
    }
  }

  /**
   * @brief Creates a `std::int64_t` JSON value (Widening conversion from
   * std::int8_t).
   */
  JsonValue(std::int8_t v) noexcept : value(static_cast<std::int64_t>(v)) {}

  /**
   * @brief Creates a `std::uint64_t` JSON value (Widening conversion from
   * std::uint8_t).
   */
  JsonValue(std::uint8_t v) noexcept : value(static_cast<std::uint64_t>(v)) {}

  /**
   * @brief Creates a `std::int64_t` JSON value (Widening conversion from
   * std::int16_t).
   */
  JsonValue(std::int16_t v) noexcept : value(static_cast<std::int64_t>(v)) {}

  /**
   * @brief Creates a `std::uint64_t` JSON value (Widening conversion from
   * std::uint16_t).
   */
  JsonValue(std::uint16_t v) noexcept : value(static_cast<std::uint64_t>(v)) {}

  /**
   * @brief Creates a `std::int64_t` JSON value (Widening conversion from
   * std::int32_t).
   */
  JsonValue(std::int32_t v) noexcept : value(static_cast<std::int64_t>(v)) {}

  /**
   * @brief Creates a `std::uint64_t` JSON value (Widening conversion from
   * std::uint32_t).
   */
  JsonValue(std::uint32_t v) noexcept : value(static_cast<std::uint64_t>(v)) {}

  /**
   * @brief Creates a `std::int64_t` JSON value.
   */
  JsonValue(std::int64_t v) noexcept : value(v) {}

  /**
   * @brief Creates a `std::uint64_t` JSON value.
   */
  JsonValue(std::uint64_t v) noexcept : value(v) {}

  /**
   * @brief Creates a `Bool` JSON value.
   */
  JsonValue(bool v) noexcept : value(v) {}

  /**
   * @brief Creates a `String` JSON value.
   */
  JsonValue(const std::string& v) : value(v) {}

  /**
   * @brief Creates a `String` JSON value.
   */
  JsonValue(std::string&& v) noexcept : value(std::move(v)) {}

  /**
   * @brief Creates a `String` JSON value.
   */
  JsonValue(const char* v) : value(std::string(v)) {}

  /**
   * @brief Creates an `Object` JSON value with the given properties.
   */
  JsonValue(const std::map<std::string, JsonValue>& v) : value(v) {}

  /**
   * @brief Creates an `Object` JSON value with the given properties.
   */
  JsonValue(std::map<std::string, JsonValue>&& v) : value(std::move(v)) {}

  /**
   * @brief Creates an `Array` JSON value with the given elements.
   */
  JsonValue(const std::vector<JsonValue>& v) : value(v) {}

  /**
   * @brief Creates an `Array` JSON value with the given elements.
   */
  JsonValue(std::vector<JsonValue>&& v) noexcept : value(std::move(v)) {}

  /**
   * @brief Creates an JSON value from the given initializer list.
   */
  JsonValue(std::initializer_list<JsonValue> v)
      : value(std::vector<JsonValue>(v)) {}

  /**
   * @brief Creates an JSON value from the given initializer list.
   */
  JsonValue(std::initializer_list<std::pair<const std::string, JsonValue>> v)
      : value(std::map<std::string, JsonValue>(v)) {}

  [[nodiscard]] const JsonValue*
  getValuePtrForKey(const std::string& key) const;
  [[nodiscard]] JsonValue* getValuePtrForKey(const std::string& key);

  /**
   * @brief Gets a typed value corresponding to the given key in the
   * object represented by this instance.
   *
   * If this instance is not a {@link JsonValue::Object}, returns
   * `nullptr`. If the key does not exist in this object, returns
   * `nullptr`. If the named value does not have the type T, returns
   * nullptr.
   *
   * @tparam T The expected type of the value.
   * @param key The key for which to retrieve the value from this object.
   * @return A pointer to the requested value, or nullptr if the value
   * cannot be obtained as requested.
   */
  template <typename T>
  const T* getValuePtrForKey(const std::string& key) const {
    const JsonValue* pValue = this->getValuePtrForKey(key);
    if (!pValue) {
      return nullptr;
    }

    return std::get_if<T>(&pValue->value);
  }

  /**
   * @brief Gets a typed value corresponding to the given key in the
   * object represented by this instance.
   *
   * If this instance is not a {@link JsonValue::Object}, returns
   * `nullptr`. If the key does not exist in this object, returns
   * `nullptr`. If the named value does not have the type T, returns
   * nullptr.
   *
   * @tparam T The expected type of the value.
   * @param key The key for which to retrieve the value from this object.
   * @return A pointer to the requested value, or nullptr if the value
   * cannot be obtained as requested.
   */
  template <typename T> T* getValuePtrForKey(const std::string& key) {
    JsonValue* pValue = this->getValuePtrForKey(key);
    return std::get_if<T>(&pValue->value);
  }

  /**
   * @brief Converts the numerical value corresponding to the given key
   * to the provided numerical template type.

   * If this instance is not a {@link JsonValue::Object}, throws
   * `std::bad_variant_access`. If the key does not exist in this object, throws
   * `JsonValueMissingKey`. If the named value does not have a numerical type T,
   *  throws `JsonValueNotRealValue`, if the named value cannot be converted
   from
   * `double` / `std::uint64_t` / `std::int64_t` without precision loss, throws
   * `gsl::narrowing_error`
   * @tparam To The expected type of the value.
   * @param key The key for which to retrieve the value from this object.
   * @return The converted value.
   * @throws If unable to convert the converted value for one of the
   aforementioned reasons.
   * @remarks Compilation will fail if type 'To' is not an integral / float /
   double type.
   */
  template <
      typename To,
      typename std::enable_if<
          std::is_integral<To>::value ||
          std::is_floating_point<To>::value>::type* = nullptr>
  [[nodiscard]] To getSafeNumericalValueForKey(const std::string& key) const {
    const Object& pObject = std::get<Object>(this->value);
    const auto it = pObject.find(key);
    if (it == pObject.end()) {
      throw JsonValueMissingKey(key);
    }
    return it->second.getSafeNumber<To>();
  }

  /**
   * @brief Converts the numerical value corresponding to the given key
   * to the provided numerical template type.
   *
   * If this instance is not a {@link JsonValue::Object}, the key does not exist
   * in this object, or the named value does not have a numerical type that can
   * be represented as T without precision loss, then the default value is
   * returned.
   *
   * @tparam To The expected type of the value.
   * @param key The key for which to retrieve the value from this object.
   * @return The converted value.
   * @throws If unable to convert the converted value for one of the
   * aforementioned reasons.
   * @remarks Compilation will fail if type 'To' is not an integral / float /
   * double type.
   */
  template <
      typename To,
      typename std::enable_if<
          std::is_integral<To>::value ||
          std::is_floating_point<To>::value>::type* = nullptr>
  [[nodiscard]] To getSafeNumericalValueOrDefaultForKey(
      const std::string& key,
      To defaultValue) const {
    const Object& pObject = std::get<Object>(this->value);
    const auto it = pObject.find(key);
    if (it == pObject.end()) {
      return defaultValue;
    }
    return it->second.getSafeNumberOrDefault<To>(defaultValue);
  }

  /**
   * @brief Determines if this value is an Object and has the given key.
   *
   * @param key The key.
   * @return true if this value contains the key. false if it is not an object
   * or does not contain the given key.
   */
  [[nodiscard]] inline bool hasKey(const std::string& key) const {
    const Object* pObject = std::get_if<Object>(&this->value);
    if (!pObject) {
      return false;
    }

    return pObject->find(key) != pObject->end();
  }

  /**
   * @brief Gets the numerical quantity from the value casted to the `To`
   * type. This function should be used over `getDouble()` / `getUint64()` /
   * `getInt64()` if you plan on casting that type into another smaller type or
   * different type.
   * @returns The converted type if it can be cast without precision loss.
   * @throws If the underlying value is not a numerical type or it cannot be
   *         converted without precision loss.
   */
  template <
      typename To,
      typename std::enable_if<
          std::is_integral<To>::value ||
          std::is_floating_point<To>::value>::type* = nullptr>
  [[nodiscard]] To getSafeNumber() const {
    const std::uint64_t* uInt = std::get_if<std::uint64_t>(&this->value);
    if (uInt) {
      return gsl::narrow<To>(*uInt);
    }

    const std::int64_t* sInt = std::get_if<std::int64_t>(&this->value);
    if (sInt) {
      return gsl::narrow<To>(*sInt);
    }

    const double* real = std::get_if<double>(&this->value);
    if (real) {
      return gsl::narrow<To>(*real);
    }

    throw JsonValueNotRealValue();
  }

  /**
   * @brief Gets the numerical quantity from the value casted to the `To`
   * type or returns defaultValue if unable to do so.

   * @returns The converted type if it can be cast without precision loss
   * or `defaultValue` if it cannot be converted safely.
   */
  template <
      typename To,
      typename std::enable_if<
          std::is_integral<To>::value ||
          std::is_floating_point<To>::value>::type* = nullptr>
  [[nodiscard]] To getSafeNumberOrDefault(To defaultValue) const noexcept {
    const std::uint64_t* uInt = std::get_if<std::uint64_t>(&this->value);
    if (uInt) {
      return losslessNarrowOrDefault<To>(*uInt, defaultValue);
    }

    const std::int64_t* sInt = std::get_if<std::int64_t>(&this->value);
    if (sInt) {
      return losslessNarrowOrDefault<To>(*sInt, defaultValue);
    }

    const double* real = std::get_if<double>(&this->value);
    if (real) {
      return losslessNarrowOrDefault<To>(*real, defaultValue);
    }

    return defaultValue;
  }

  /**
   * @brief Gets the object from the value.
   * @return The object.
   * @throws std::bad_variant_access if the underlying type is not a
   * JsonValue::Object
   */

  [[nodiscard]] inline const JsonValue::Object& getObject() const {
    return std::get<JsonValue::Object>(this->value);
  }

  /**
   * @brief Gets the string from the value.
   * @return The string.
   * @throws std::bad_variant_access if the underlying type is not a
   * JsonValue::String
   */
  [[nodiscard]] inline const JsonValue::String& getString() const {
    return std::get<String>(this->value);
  }

  /**
   * @brief Gets the array from the value.
   * @return The array.
   * @throws std::bad_variant_access if the underlying type is not a
   * JsonValue::Array
   */
  [[nodiscard]] inline const JsonValue::Array& getArray() const {
    return std::get<JsonValue::Array>(this->value);
  }

  /**
   * @brief Gets an array of strings from the value.
   *
   * @param defaultString The default string to include in the array for an
   * element that is not a string.
   * @return The array of strings, or an empty array if this value is not an
   * array at all.
   */
  [[nodiscard]] std::vector<std::string>
  getArrayOfStrings(const std::string& defaultString) const;

  /**
   * @brief Gets the bool from the value.
   * @return The bool.
   * @throws std::bad_variant_access if the underlying type is not a
   * JsonValue::Bool
   */
  [[nodiscard]] inline bool getBool() const {
    return std::get<bool>(this->value);
  }

  /**
   * @brief Gets the double from the value.
   * @return The double.
   * @throws std::bad_variant_access if the underlying type is not a double
   */
  [[nodiscard]] inline double getDouble() const {
    return std::get<double>(this->value);
  }

  /**
   * @brief Gets the std::uint64_t from the value.
   * @return The std::uint64_t.
   * @throws std::bad_variant_access if the underlying type is not a
   * std::uint64_t
   */
  [[nodiscard]] std::uint64_t getUint64() const {
    return std::get<std::uint64_t>(this->value);
  }

  /**
   * @brief Gets the std::int64_t from the value.
   * @return The std::int64_t.
   * @throws std::bad_variant_access if the underlying type is not a
   * std::int64_t
   */
  [[nodiscard]] std::int64_t getInt64() const {
    return std::get<std::int64_t>(this->value);
  }

  /**
   * @brief Gets the bool from the value or returns defaultValue
   * @return The bool or defaultValue if this->value is not a bool.
   */
  [[nodiscard]] inline bool getBoolOrDefault(bool defaultValue) const noexcept {
    const auto* v = std::get_if<bool>(&this->value);
    if (v) {
      return *v;
    }

    return defaultValue;
  }

  /**
   * @brief Gets the string from the value or returns defaultValue
   * @return The string or defaultValue if this->value is not a string.
   */
  [[nodiscard]] inline const JsonValue::String
  getStringOrDefault(String defaultValue) const {
    const auto* v = std::get_if<JsonValue::String>(&this->value);
    if (v) {
      return *v;
    }

    return defaultValue;
  }

  /**
   * @brief Gets the double from the value or returns defaultValue
   * @return The double or defaultValue if this->value is not a double.
   */
  [[nodiscard]] inline double
  getDoubleOrDefault(double defaultValue) const noexcept {
    const auto* v = std::get_if<double>(&this->value);
    if (v) {
      return *v;
    }

    return defaultValue;
  }

  /**
   * @brief Gets the uint64_t from the value or returns defaultValue
   * @return The uint64_t or defaultValue if this->value is not a uint64_t.
   */
  [[nodiscard]] inline std::uint64_t
  getUint64OrDefault(std::uint64_t defaultValue) const noexcept {
    const auto* v = std::get_if<std::uint64_t>(&this->value);
    if (v) {
      return *v;
    }

    return defaultValue;
  }

  /**
   * @brief Gets the int64_t from the value or returns defaultValue
   * @return The int64_t or defaultValue if this->value is not a int64_t.
   */
  [[nodiscard]] inline std::int64_t
  getInt64OrDefault(std::int64_t defaultValue) const noexcept {
    const auto* v = std::get_if<std::int64_t>(&this->value);
    if (v) {
      return *v;
    }

    return defaultValue;
  }

  /**
   * @brief Returns whether this value is a `null` value.
   */
  [[nodiscard]] inline bool isNull() const noexcept {
    return std::holds_alternative<Null>(this->value);
  }

  /**
   * @brief Returns whether this value is a `double`, `std::uint64_t` or
   * `std::int64_t`. Use this function in conjunction with `getNumber` for
   *  safely casting to arbitrary types
   */
  [[nodiscard]] inline bool isNumber() const noexcept {
    return isDouble() || isUint64() || isInt64();
  }

  /**
   * @brief Returns whether this value is a `Bool` value.
   */
  [[nodiscard]] inline bool isBool() const noexcept {
    return std::holds_alternative<Bool>(this->value);
  }

  /**
   * @brief Returns whether this value is a `String` value.
   */
  [[nodiscard]] inline bool isString() const noexcept {
    return std::holds_alternative<String>(this->value);
  }

  /**
   * @brief Returns whether this value is an `Object` value.
   */
  [[nodiscard]] inline bool isObject() const noexcept {
    return std::holds_alternative<Object>(this->value);
  }

  /**
   * @brief Returns whether this value is an `Array` value.
   */
  [[nodiscard]] inline bool isArray() const noexcept {
    return std::holds_alternative<Array>(this->value);
  }

  /**
   * @brief Returns whether this value is a `double` value.
   */
  [[nodiscard]] inline bool isDouble() const noexcept {
    return std::holds_alternative<double>(this->value);
  }

  /**
   * @brief Returns whether this value is a `std::uint64_t` value.
   */
  [[nodiscard]] inline bool isUint64() const noexcept {
    return std::holds_alternative<std::uint64_t>(this->value);
  }

  /**
   * @brief Returns whether this value is a `std::int64_t` value.
   */
  [[nodiscard]] inline bool isInt64() const noexcept {
    return std::holds_alternative<std::int64_t>(this->value);
  }

  /**
   * @brief The actual value.
   *
   * The type of the value may be queried with the `isNull`, `isDouble`,
   * `isBool`, `isString`, `isObject`, `isUint64`, `isInt64`, `isNumber`, and
   * `isArray` functions.
   *
   * The actual value may be obtained with the `getNumber`, `getBool`,
   * and `getString` functions for the respective types. For
   * `Object` values, the properties may be accessed with the
   * `getValueForKey` functions.
   */
  std::variant<
      Null,
      double,
      std::uint64_t,
      std::int64_t,
      Bool,
      String,
      Object,
      Array>
      value;
};
} // namespace CesiumUtility
