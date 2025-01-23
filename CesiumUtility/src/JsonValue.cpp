#include <CesiumUtility/JsonValue.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace CesiumUtility {

namespace {

// We're defining these instead of using the ones in <concepts> in order to
// support Android NDK r25b which doesn't have a working <concepts> header.
template <typename T>
concept integral = std::is_integral_v<T>;
template <typename T>
concept signed_integral = integral<T> && std::is_signed_v<T>;
template <typename T>
concept unsigned_integral = integral<T> && std::is_unsigned_v<T>;
template <typename T>
concept floating_point = std::is_floating_point_v<T>;

template <typename TTo, typename TFrom> struct LosslessNarrower {
  static std::optional<TTo> losslessNarrow(TFrom from) noexcept;
};

// No-op conversion of a type to itself.
template <typename T> struct LosslessNarrower<T, T> {
  static std::optional<T> losslessNarrow(T from) noexcept { return from; }
};

// Conversion of float to double is no problem.
template <> struct LosslessNarrower<double, float> {
  static std::optional<double> losslessNarrow(float from) noexcept {
    return static_cast<double>(from);
  }
};

// Conversion of double to float must not lose precision
template <> struct LosslessNarrower<float, double> {
  static std::optional<float> losslessNarrow(double from) noexcept {
    std::optional<float> asFloat = std::make_optional(static_cast<float>(from));
    if (static_cast<double>(*asFloat) == from ||
        (std::isnan(*asFloat) && std::isnan(from))) {
      // Value or NaN successfully converted.
      return asFloat;
    } else {
      return std::nullopt;
    }
  }
};

// Conversion of a floating-point value to an integer. The floating point number
// must be integer-valued and within the range of the integer type.
template <integral TTo, floating_point TFrom>
struct LosslessNarrower<TTo, TFrom> {
  static std::optional<TTo> losslessNarrow(TFrom from) noexcept {
    TTo min = std::numeric_limits<TTo>::lowest();
    TTo max = std::numeric_limits<TTo>::max();
    if (from >= static_cast<TFrom>(min) && from <= static_cast<TFrom>(max)) {
      std::optional<TTo> casted = std::make_optional(static_cast<TTo>(from));
      if (static_cast<TFrom>(*casted) == from) {
        return casted;
      }
    }
    return std::nullopt;
  }
};

// Conversion of an integer to a floating point value. All integers are within
// range of both float and double, but we need to make sure the conversion is
// lossless.
template <floating_point TTo, integral TFrom>
struct LosslessNarrower<TTo, TFrom> {
  static std::optional<TTo> losslessNarrow(TFrom from) noexcept {
    std::optional<TTo> casted = std::make_optional(static_cast<TTo>(from));

    if (static_cast<TFrom>(*casted) == from) {
      return casted;
    }

    return std::nullopt;
  }
};

// Conversion between signed integers. If TFrom is bigger the value must be in
// range of TTo.
template <signed_integral TTo, signed_integral TFrom>
struct LosslessNarrower<TTo, TFrom> {
  static std::optional<TTo> losslessNarrow(TFrom from) noexcept {
    if constexpr (sizeof(TFrom) > sizeof(TTo)) {
      TTo min = std::numeric_limits<TTo>::lowest();
      TTo max = std::numeric_limits<TTo>::max();
      if (from < static_cast<TFrom>(min) || from > static_cast<TFrom>(max)) {
        return std::nullopt;
      }
    }

    return std::make_optional(static_cast<TTo>(from));
  }
};

// Conversion between unsigned integers. Identical to conversion between signed
// integers.
template <unsigned_integral TTo, unsigned_integral TFrom>
struct LosslessNarrower<TTo, TFrom> {
  static constexpr std::optional<TTo> losslessNarrow(TFrom from) noexcept {
    if constexpr (sizeof(TFrom) > sizeof(TTo)) {
      TTo min = std::numeric_limits<TTo>::lowest();
      TTo max = std::numeric_limits<TTo>::max();
      if (from < static_cast<TFrom>(min) || from > static_cast<TFrom>(max)) {
        return std::nullopt;
      }
    }

    return std::make_optional(static_cast<TTo>(from));
  }
};

// Conversion of signed to unsigned integer
template <unsigned_integral TTo, signed_integral TFrom>
struct LosslessNarrower<TTo, TFrom> {
  static std::optional<TTo> losslessNarrow(TFrom from) noexcept {
    // Can't convert a value less than zero to unsigned.
    if (from < TFrom(0)) {
      return std::nullopt;
    }

    // All positive signed values can convert to an unsigned value of the same
    // size or larger. But if TTo is smaller, make sure the from value is <= its
    // max.
    if constexpr (sizeof(TFrom) > sizeof(TTo)) {
      TTo max = std::numeric_limits<TTo>::max();
      if (from > static_cast<TFrom>(max)) {
        return std::nullopt;
      }
    }

    return std::make_optional(static_cast<TTo>(from));
  }
};

// Conversion of unsigned to signed integer
template <signed_integral TTo, unsigned_integral TFrom>
struct LosslessNarrower<TTo, TFrom> {
  static std::optional<TTo> losslessNarrow(TFrom from) noexcept {
    // Conversion to a larger type is always fine.
    // For an equal or smaller type, make sure the `from` value is not greater
    // than the `to` type's max.
    if constexpr (sizeof(TTo) <= sizeof(TFrom)) {
      TTo max = std::numeric_limits<TTo>::max();
      if (from > static_cast<TFrom>(max)) {
        return std::nullopt;
      }
    }

    return std::make_optional(static_cast<TTo>(from));
  }
};

} // namespace

template <typename TTo, typename TFrom>
std::optional<TTo> losslessNarrow(TFrom from) noexcept {
  return LosslessNarrower<TTo, TFrom>::losslessNarrow(from);
}

// Explicit instantiations for all the possible combinations.
template std::optional<double> losslessNarrow<double, double>(double);
template std::optional<double> losslessNarrow<double, float>(float);
template std::optional<double> losslessNarrow<double, int8_t>(int8_t);
template std::optional<double> losslessNarrow<double, int16_t>(int16_t);
template std::optional<double> losslessNarrow<double, int32_t>(int32_t);
template std::optional<double> losslessNarrow<double, int64_t>(int64_t);
template std::optional<double> losslessNarrow<double, uint8_t>(uint8_t);
template std::optional<double> losslessNarrow<double, uint16_t>(uint16_t);
template std::optional<double> losslessNarrow<double, uint32_t>(uint32_t);
template std::optional<double> losslessNarrow<double, uint64_t>(uint64_t);

template std::optional<float> losslessNarrow<float, double>(double);
template std::optional<float> losslessNarrow<float, float>(float);
template std::optional<float> losslessNarrow<float, int8_t>(int8_t);
template std::optional<float> losslessNarrow<float, int16_t>(int16_t);
template std::optional<float> losslessNarrow<float, int32_t>(int32_t);
template std::optional<float> losslessNarrow<float, int64_t>(int64_t);
template std::optional<float> losslessNarrow<float, uint8_t>(uint8_t);
template std::optional<float> losslessNarrow<float, uint16_t>(uint16_t);
template std::optional<float> losslessNarrow<float, uint32_t>(uint32_t);
template std::optional<float> losslessNarrow<float, uint64_t>(uint64_t);

template std::optional<int8_t> losslessNarrow<int8_t, double>(double);
template std::optional<int8_t> losslessNarrow<int8_t, float>(float);
template std::optional<int8_t> losslessNarrow<int8_t, int8_t>(int8_t);
template std::optional<int8_t> losslessNarrow<int8_t, int16_t>(int16_t);
template std::optional<int8_t> losslessNarrow<int8_t, int32_t>(int32_t);
template std::optional<int8_t> losslessNarrow<int8_t, int64_t>(int64_t);
template std::optional<int8_t> losslessNarrow<int8_t, uint8_t>(uint8_t);
template std::optional<int8_t> losslessNarrow<int8_t, uint16_t>(uint16_t);
template std::optional<int8_t> losslessNarrow<int8_t, uint32_t>(uint32_t);
template std::optional<int8_t> losslessNarrow<int8_t, uint64_t>(uint64_t);

template std::optional<int16_t> losslessNarrow<int16_t, double>(double);
template std::optional<int16_t> losslessNarrow<int16_t, float>(float);
template std::optional<int16_t> losslessNarrow<int16_t, int8_t>(int8_t);
template std::optional<int16_t> losslessNarrow<int16_t, int16_t>(int16_t);
template std::optional<int16_t> losslessNarrow<int16_t, int32_t>(int32_t);
template std::optional<int16_t> losslessNarrow<int16_t, int64_t>(int64_t);
template std::optional<int16_t> losslessNarrow<int16_t, uint8_t>(uint8_t);
template std::optional<int16_t> losslessNarrow<int16_t, uint16_t>(uint16_t);
template std::optional<int16_t> losslessNarrow<int16_t, uint32_t>(uint32_t);
template std::optional<int16_t> losslessNarrow<int16_t, uint64_t>(uint64_t);

template std::optional<int32_t> losslessNarrow<int32_t, double>(double);
template std::optional<int32_t> losslessNarrow<int32_t, float>(float);
template std::optional<int32_t> losslessNarrow<int32_t, int8_t>(int8_t);
template std::optional<int32_t> losslessNarrow<int32_t, int16_t>(int16_t);
template std::optional<int32_t> losslessNarrow<int32_t, int32_t>(int32_t);
template std::optional<int32_t> losslessNarrow<int32_t, int64_t>(int64_t);
template std::optional<int32_t> losslessNarrow<int32_t, uint8_t>(uint8_t);
template std::optional<int32_t> losslessNarrow<int32_t, uint16_t>(uint16_t);
template std::optional<int32_t> losslessNarrow<int32_t, uint32_t>(uint32_t);
template std::optional<int32_t> losslessNarrow<int32_t, uint64_t>(uint64_t);

template std::optional<int64_t> losslessNarrow<int64_t, double>(double);
template std::optional<int64_t> losslessNarrow<int64_t, float>(float);
template std::optional<int64_t> losslessNarrow<int64_t, int8_t>(int8_t);
template std::optional<int64_t> losslessNarrow<int64_t, int16_t>(int16_t);
template std::optional<int64_t> losslessNarrow<int64_t, int32_t>(int32_t);
template std::optional<int64_t> losslessNarrow<int64_t, int64_t>(int64_t);
template std::optional<int64_t> losslessNarrow<int64_t, uint8_t>(uint8_t);
template std::optional<int64_t> losslessNarrow<int64_t, uint16_t>(uint16_t);
template std::optional<int64_t> losslessNarrow<int64_t, uint32_t>(uint32_t);
template std::optional<int64_t> losslessNarrow<int64_t, uint64_t>(uint64_t);

template std::optional<uint8_t> losslessNarrow<uint8_t, double>(double);
template std::optional<uint8_t> losslessNarrow<uint8_t, float>(float);
template std::optional<uint8_t> losslessNarrow<uint8_t, int8_t>(int8_t);
template std::optional<uint8_t> losslessNarrow<uint8_t, int16_t>(int16_t);
template std::optional<uint8_t> losslessNarrow<uint8_t, int32_t>(int32_t);
template std::optional<uint8_t> losslessNarrow<uint8_t, int64_t>(int64_t);
template std::optional<uint8_t> losslessNarrow<uint8_t, uint8_t>(uint8_t);
template std::optional<uint8_t> losslessNarrow<uint8_t, uint16_t>(uint16_t);
template std::optional<uint8_t> losslessNarrow<uint8_t, uint32_t>(uint32_t);
template std::optional<uint8_t> losslessNarrow<uint8_t, uint64_t>(uint64_t);

template std::optional<uint16_t> losslessNarrow<uint16_t, double>(double);
template std::optional<uint16_t> losslessNarrow<uint16_t, float>(float);
template std::optional<uint16_t> losslessNarrow<uint16_t, int8_t>(int8_t);
template std::optional<uint16_t> losslessNarrow<uint16_t, int16_t>(int16_t);
template std::optional<uint16_t> losslessNarrow<uint16_t, int32_t>(int32_t);
template std::optional<uint16_t> losslessNarrow<uint16_t, int64_t>(int64_t);
template std::optional<uint16_t> losslessNarrow<uint16_t, uint8_t>(uint8_t);
template std::optional<uint16_t> losslessNarrow<uint16_t, uint16_t>(uint16_t);
template std::optional<uint16_t> losslessNarrow<uint16_t, uint32_t>(uint32_t);
template std::optional<uint16_t> losslessNarrow<uint16_t, uint64_t>(uint64_t);

template std::optional<uint32_t> losslessNarrow<uint32_t, double>(double);
template std::optional<uint32_t> losslessNarrow<uint32_t, float>(float);
template std::optional<uint32_t> losslessNarrow<uint32_t, int8_t>(int8_t);
template std::optional<uint32_t> losslessNarrow<uint32_t, int16_t>(int16_t);
template std::optional<uint32_t> losslessNarrow<uint32_t, int32_t>(int32_t);
template std::optional<uint32_t> losslessNarrow<uint32_t, int64_t>(int64_t);
template std::optional<uint32_t> losslessNarrow<uint32_t, uint8_t>(uint8_t);
template std::optional<uint32_t> losslessNarrow<uint32_t, uint16_t>(uint16_t);
template std::optional<uint32_t> losslessNarrow<uint32_t, uint32_t>(uint32_t);
template std::optional<uint32_t> losslessNarrow<uint32_t, uint64_t>(uint64_t);

template std::optional<uint64_t> losslessNarrow<uint64_t, double>(double);
template std::optional<uint64_t> losslessNarrow<uint64_t, float>(float);
template std::optional<uint64_t> losslessNarrow<uint64_t, int8_t>(int8_t);
template std::optional<uint64_t> losslessNarrow<uint64_t, int16_t>(int16_t);
template std::optional<uint64_t> losslessNarrow<uint64_t, int32_t>(int32_t);
template std::optional<uint64_t> losslessNarrow<uint64_t, int64_t>(int64_t);
template std::optional<uint64_t> losslessNarrow<uint64_t, uint8_t>(uint8_t);
template std::optional<uint64_t> losslessNarrow<uint64_t, uint16_t>(uint16_t);
template std::optional<uint64_t> losslessNarrow<uint64_t, uint32_t>(uint32_t);
template std::optional<uint64_t> losslessNarrow<uint64_t, uint64_t>(uint64_t);

const JsonValue* JsonValue::getValuePtrForKey(const std::string& key) const {
  const Object* pObject = std::get_if<Object>(&this->value);
  if (!pObject) {
    return nullptr;
  }

  auto it = pObject->find(key);
  if (it == pObject->end()) {
    return nullptr;
  }

  return &it->second;
}

JsonValue* JsonValue::getValuePtrForKey(const std::string& key) {
  Object* pObject = std::get_if<Object>(&this->value);
  if (!pObject) {
    return nullptr;
  }

  auto it = pObject->find(key);
  if (it == pObject->end()) {
    return nullptr;
  }

  return &it->second;
}

} // namespace CesiumUtility
std::vector<std::string> CesiumUtility::JsonValue::getArrayOfStrings(
    const std::string& defaultString) const {
  if (!this->isArray())
    return std::vector<std::string>();

  const JsonValue::Array& array = this->getArray();
  std::vector<std::string> result(array.size());
  std::transform(
      array.begin(),
      array.end(),
      result.begin(),
      [&defaultString](const JsonValue& arrayValue) {
        return arrayValue.getStringOrDefault(defaultString);
      });
  return result;
}
