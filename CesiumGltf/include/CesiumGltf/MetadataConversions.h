#pragma once

#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumUtility/JsonValue.h>

#include <glm/common.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cerrno>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace CesiumGltf {
/**
 * @brief Default conversion between two types. No actual conversion is defined.
 * This returns std::nullopt to indicate the conversion was not successful.
 */
template <typename TTo, typename TFrom, typename Enable = void>
struct MetadataConversions {
  /**
   * @brief Converts between `TFrom` and `TTo` where no actual conversion is
   * defined, returning `std::nullopt`.
   */
  static std::optional<TTo> convert(TFrom /*from*/) { return std::nullopt; }
};

/**
 * @brief Trivially converts any type to itself.
 */
template <typename T> struct MetadataConversions<T, T> {
  /**
   * @brief Converts an instance of `T` to an instance of `T`, always returning
   * the same value that was passed in.
   */
  static std::optional<T> convert(T from) { return from; }
};

#pragma region Conversions to boolean
/**
 * @brief Converts from a scalar to a bool.
 */
template <typename TFrom>
struct MetadataConversions<
    bool,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataScalar<TFrom>::value>> {
  /**
   * @brief Converts a scalar to a boolean. Zero is converted to false, while
   * nonzero values are converted to true.
   *
   * @param from The scalar to convert from.
   */
  static std::optional<bool> convert(TFrom from) {
    return from != static_cast<TFrom>(0);
  }
};

/**
 * @brief Converts from std::string_view to a bool.
 */
template <> struct MetadataConversions<bool, std::string_view> {
private:
  static bool
  isEqualCaseInsensitive(const std::string_view& a, const std::string_view& b) {
    if (a.size() != b.size()) {
      return false;
    }

    for (size_t i = 0; i < a.size(); i++) {
      if (std::tolower(a[i]) != std::tolower(b[i])) {
        return false;
      }
    }
    return true;
  }

public:
  /**
   * @brief Converts the contents of a std::string_view to a boolean.
   *
   * "0", "false", and "no" (case-insensitive) are converted to false, while
   * "1", "true", and "yes" are converted to true. All other strings will return
   * std::nullopt.
   *
   * @param from The std::string_view to convert from.
   */
  static std::optional<bool> convert(const std::string_view& from) {
    if (isEqualCaseInsensitive(from, "1") ||
        isEqualCaseInsensitive(from, "true") ||
        isEqualCaseInsensitive(from, "yes")) {
      return true;
    }

    if (isEqualCaseInsensitive(from, "0") ||
        isEqualCaseInsensitive(from, "false") ||
        isEqualCaseInsensitive(from, "no")) {
      return false;
    }

    return std::nullopt;
  }
};

/**
 * @brief Converts from std::string to a bool.
 */
template <> struct MetadataConversions<bool, std::string> {
public:
  /**
   * @brief Converts the contents of a std::string to a boolean.
   *
   * "0", "false", and "no" (case-insensitive) are converted to false, while
   * "1", "true", and "yes" are converted to true. All other strings will return
   * std::nullopt.
   *
   * @param from The std::string to convert from.
   */
  static std::optional<bool> convert(const std::string& from) {
    return MetadataConversions<bool, std::string_view>::convert(
        std::string_view(from.data(), from.size()));
  }
};

#pragma endregion

#pragma region Conversions to integer
/**
 * @brief Converts from one integer type to another.
 */
template <typename TTo, typename TFrom>
struct MetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value &&
        CesiumGltf::IsMetadataInteger<TFrom>::value &&
        !std::is_same_v<TTo, TFrom>>> {
  /**
   * @brief Converts a value of the given integer to another integer type. If
   * the integer cannot be losslessly converted to the desired type,
   * std::nullopt is returned.
   *
   * @param from The integer value to convert from.
   */
  static std::optional<TTo> convert(TFrom from) {
    return CesiumUtility::losslessNarrow<TTo, TFrom>(from);
  }
};

/**
 * @brief Converts from a floating-point type to an integer.
 */
template <typename TTo, typename TFrom>
struct MetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value &&
        CesiumGltf::IsMetadataFloating<TFrom>::value>> {
  /**
   * @brief Converts a floating-point value to an integer type. This truncates
   * the floating-point value, rounding it towards zero.
   *
   * If the value is outside the range of the integer type, std::nullopt is
   * returned.
   *
   * @param from The floating-point value to convert from.
   */
  static std::optional<TTo> convert(TFrom from) {
    if (double(std::numeric_limits<TTo>::max()) < from ||
        double(std::numeric_limits<TTo>::lowest()) > from) {
      // Floating-point number is outside the range of this integer type.
      return std::nullopt;
    }

    return static_cast<TTo>(from);
  }
};

/**
 * @brief Converts from std::string to a signed integer.
 */
template <typename TTo>
struct MetadataConversions<
    TTo,
    std::string,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value && std::is_signed_v<TTo>>> {
  /**
   * @brief Converts the contents of a std::string to a signed integer.
   * This assumes that the entire std::string represents the number, not
   * just a part of it.
   *
   * This returns std::nullopt if no number is parsed from the string.
   *
   * @param from The std::string to parse from.
   */
  static std::optional<TTo> convert(const std::string& from) {
    if (from.size() == 0) {
      // Return early. Otherwise, empty strings will be parsed as 0, which is
      // misleading.
      return std::nullopt;
    }

    errno = 0;

    char* pLastUsed;
    int64_t parsedValue = std::strtoll(from.c_str(), &pLastUsed, 10);
    if (errno == 0 && pLastUsed == from.c_str() + from.size()) {
      // Successfully parsed the entire string as an integer of this type.
      return CesiumUtility::losslessNarrow<TTo, int64_t>(parsedValue);
    }

    errno = 0;

    // Failed to parse as an integer. Maybe we can parse as a double and
    // truncate it?
    double parsedDouble = std::strtod(from.c_str(), &pLastUsed);
    if (errno == 0 && pLastUsed == from.c_str() + from.size()) {
      // Successfully parsed the entire string as a double.
      // Convert it to an integer if we can.
      double truncated = glm::trunc(parsedDouble);

      int64_t asInteger = static_cast<int64_t>(truncated);
      double roundTrip = static_cast<double>(asInteger);
      if (roundTrip == truncated) {
        return CesiumUtility::losslessNarrow<TTo, int64_t>(asInteger);
      }
    }

    return std::nullopt;
  }
}; // namespace CesiumGltf

/**
 * @brief Converts from std::string to an unsigned integer.
 */
template <typename TTo>
struct MetadataConversions<
    TTo,
    std::string,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value && !std::is_signed_v<TTo>>> {
  /**
   * @brief Converts the contents of a std::string to an unsigned integer.
   * This assumes that the entire std::string represents the number, not
   * just a part of it.
   *
   * This returns std::nullopt if no number is parsed from the string.
   *
   * @param from The std::string to parse from.
   */
  static std::optional<TTo> convert(const std::string& from) {
    if (from.size() == 0) {
      // Return early. Otherwise, empty strings will be parsed as 0, which is
      // misleading.
      return std::nullopt;
    }

    if (from.find('-') != std::string::npos) {
      // The string must be manually checked for a negative sign because for
      // std::strtoull accepts negative numbers and bitcasts them, which is not
      // desired!
      return std::nullopt;
    }

    errno = 0;

    char* pLastUsed;
    uint64_t parsedValue = std::strtoull(from.c_str(), &pLastUsed, 10);
    if (errno == 0 && pLastUsed == from.c_str() + from.size()) {
      // Successfully parsed the entire string as an integer of this type.
      return CesiumUtility::losslessNarrow<TTo, uint64_t>(parsedValue);
    }

    // Failed to parse as an integer. Maybe we can parse as a double and
    // truncate it?
    errno = 0;

    double parsedDouble = std::strtod(from.c_str(), &pLastUsed);
    if (errno == 0 && pLastUsed == from.c_str() + from.size()) {
      // Successfully parsed the entire string as a double.
      // Convert it to an integer if we can.
      double truncated = glm::trunc(parsedDouble);

      uint64_t asInteger = static_cast<uint64_t>(truncated);
      double roundTrip = static_cast<double>(asInteger);
      if (roundTrip == truncated) {
        return CesiumUtility::losslessNarrow<TTo, uint64_t>(asInteger);
      }
    }

    return std::nullopt;
  }
};

/**
 * @brief Converts from std::string_view to an integer.
 */
template <typename TTo>
struct MetadataConversions<
    TTo,
    std::string_view,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TTo>::value>> {
  /**
   * @brief Converts the contents of a std::string_view to an integer.
   * This assumes that the entire std::string_view represents the number, not
   * just a part of it.
   *
   * This returns std::nullopt if no number is parsed from the string.
   *
   * @param from The std::string_view to parse from.
   */
  static std::optional<TTo> convert(const std::string_view& from) {
    if (from.size() == 0) {
      // Return early. Otherwise, empty strings will be parsed as 0, which is
      // misleading.
      return std::nullopt;
    }

    // Amazingly, C++ has no* string parsing functions that work with strings
    // that might not be null-terminated. So we have to copy to a std::string
    // (which _is_ guaranteed to be null terminated) before parsing.
    // * except std::from_chars, but compiler/library support for the
    //   floating-point version of that method is spotty at best.
    return MetadataConversions<TTo, std::string>::convert(std::string(from));
  }
};

/**
 * @brief Converts from a boolean to an integer type.
 */
template <typename TTo>
struct MetadataConversions<
    TTo,
    bool,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TTo>::value>> {
  /**
   * @brief Converts a boolean to an integer. This returns 1 for true, 0 for
   * false.
   *
   * @param from The boolean value to be converted.
   */
  static std::optional<TTo> convert(bool from) { return from ? 1 : 0; }
};
#pragma endregion

#pragma region Conversions to float
/**
 * @brief Converts from a boolean to a float.
 */
template <> struct MetadataConversions<float, bool> {
  /**
   * @brief Converts a boolean to a float. This returns 1.0f for true, 0.0f for
   * false.
   *
   * @param from The boolean value to be converted.
   */
  static std::optional<float> convert(bool from) { return from ? 1.0f : 0.0f; }
};

/**
 * @brief Converts from an integer type to a float.
 */
template <typename TFrom>
struct MetadataConversions<
    float,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
  /**
   * @brief Converts an integer to a float. The value may lose precision during
   * conversion.
   *
   * @param from The integer value to be converted.
   */
  static std::optional<float> convert(TFrom from) {
    return static_cast<float>(from);
  }
};

/**
 * @brief Converts from a double to a float.
 */
template <> struct MetadataConversions<float, double> {
  /**
   * @brief Converts a double to a float. The value may lose precision during
   * conversion.
   *
   * If the value is outside the range of a float, std::nullopt is returned.
   *
   * @param from The double value to be converted.
   */
  static std::optional<float> convert(double from) {
    if (from > std::numeric_limits<float>::max() ||
        from < std::numeric_limits<float>::lowest()) {
      return std::nullopt;
    }
    return static_cast<float>(from);
  }
};

/**
 * @brief Converts from a std::string to a float.
 */
template <> struct MetadataConversions<float, std::string> {
  /**
   * @brief Converts a std::string to a float. This assumes that the entire
   * std::string represents the number, not just a part of it.
   *
   * This returns std::nullopt if no number is parsed from the string.
   *
   * @param from The std::string to parse from.
   */
  static std::optional<float> convert(const std::string& from) {
    if (from.size() == 0) {
      // Return early. Otherwise, empty strings will be parsed as 0, which is
      // misleading.
      return std::nullopt;
    }

    errno = 0;

    char* pLastUsed;
    float parsedValue = std::strtof(from.c_str(), &pLastUsed);
    if (errno == 0 && pLastUsed == from.c_str() + from.size() &&
        !std::isinf(parsedValue)) {
      // Successfully parsed the entire string as a float.
      return parsedValue;
    }
    return std::nullopt;
  }
};

/**
 * @brief Converts from a std::string_view to a float.
 */
template <> struct MetadataConversions<float, std::string_view> {
  /**
   * @brief Converts a std::string_view to a float. This assumes that the entire
   * std::string_view represents the number, not just a part of it.
   *
   * This returns std::nullopt if no number is parsed from the string.
   *
   * @param from The std::string_view to parse from.
   */
  static std::optional<float> convert(const std::string_view& from) {
    if (from.size() == 0) {
      // Return early. Otherwise, empty strings will be parsed as 0, which is
      // misleading.
      return std::nullopt;
    }
    // Amazingly, C++ has no* string parsing functions that work with strings
    // that might not be null-terminated. So we have to copy to a std::string
    // (which _is_ guaranteed to be null terminated) before parsing.
    // * except std::from_chars, but compiler/library support for the
    //   floating-point version of that method is spotty at best.
    return MetadataConversions<float, std::string>::convert(
        std::string(from.data(), from.size()));
  }
};
#pragma endregion

#pragma region Conversions to double
/**
 * @brief Converts from a boolean to a double.
 */
template <> struct MetadataConversions<double, bool> {
  /**
   * @brief Converts a boolean to a double. This returns 1.0 for true, 0.0 for
   * false.
   *
   * @param from The boolean value to be converted.
   */
  static std::optional<double> convert(bool from) { return from ? 1.0 : 0.0; }
};

/**
 * @brief Converts from any integer type to a double.
 */
template <typename TFrom>
struct MetadataConversions<
    double,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
  /**
   * @brief Converts any integer type to a double. The value may lose precision
   * during conversion.
   *
   * @param from The boolean value to be converted.
   */
  static std::optional<double> convert(TFrom from) {
    return static_cast<double>(from);
  }
};

/**
 * @brief Converts from a float to a double.
 */
template <> struct MetadataConversions<double, float> {
  /**
   * @brief Converts from a float to a double.
   *
   * @param from The float value to be converted.
   */
  static std::optional<double> convert(float from) {
    return static_cast<double>(from);
  }
};

/**
 * @brief Converts from std::string to a double.
 */
template <> struct MetadataConversions<double, std::string> {
  /**
   * Converts a std::string to a double. This assumes that the entire
   * std::string represents the number, not just a part of it.
   *
   * This returns std::nullopt if no number is parsed from the string.
   *
   * @param from The std::string to parse from.
   */
  static std::optional<double> convert(const std::string& from) {
    if (from.size() == 0) {
      // Return early. Otherwise, empty strings will be parsed as 0, which is
      // misleading.
      return std::nullopt;
    }

    errno = 0;

    char* pLastUsed;
    double parsedValue = std::strtod(from.c_str(), &pLastUsed);
    if (errno == 0 && pLastUsed == from.c_str() + from.size() &&
        !std::isinf(parsedValue)) {
      // Successfully parsed the entire string as a double.
      return parsedValue;
    }
    return std::nullopt;
  }
};

/**
 * @brief Converts from std::string_view to a double.
 */
template <> struct MetadataConversions<double, std::string_view> {
  /**
   * Converts a std::string_view to a double. This assumes that the entire
   * std::string_view represents the number, not just a part of it.
   *
   * This returns std::nullopt if no number is parsed from the string.
   *
   * @param from The std::string_view to parse from.
   */
  static std::optional<double> convert(const std::string_view& from) {
    if (from.size() == 0) {
      // Return early. Otherwise, empty strings will be parsed as 0, which is
      // misleading.
      return std::nullopt;
    }

    // Amazingly, C++ has no* string parsing functions that work with strings
    // that might not be null-terminated. So we have to copy to a std::string
    // (which _is_ guaranteed to be null terminated) before parsing.
    // * except std::from_chars, but compiler/library support for the
    //   floating-point version of that method is spotty at best.
    return MetadataConversions<double, std::string>::convert(std::string(from));
  }
};
#pragma endregion

#pragma region Conversions to string
/**
 * @brief Converts from a boolean to a string.
 */
template <> struct MetadataConversions<std::string, bool> {
  /**
   * @brief Converts a boolean to a std::string. Returns "true" for true and
   * "false" for false.
   *
   * @param from The boolean to be converted.
   */
  static std::optional<std::string> convert(bool from) {
    return from ? "true" : "false";
  }
};

/**
 * @brief Converts from a scalar to a string.
 */
template <typename TFrom>
struct MetadataConversions<
    std::string,
    TFrom,
    std::enable_if_t<IsMetadataScalar<TFrom>::value>> {
  /**
   * @brief Converts a scalar to a std::string.
   *
   * @param from The scalar to be converted.
   */
  static std::optional<std::string> convert(TFrom from) {
    return std::to_string(from);
  }
};

/**
 * @brief Converts from a glm::vecN or glm::matN to a string.
 */
template <typename TFrom>
struct MetadataConversions<
    std::string,
    TFrom,
    std::enable_if_t<
        IsMetadataVecN<TFrom>::value || IsMetadataMatN<TFrom>::value>> {
  /**
   * @brief Converts a glm::vecN or glm::matN to a std::string. This uses the
   * format that glm::to_string() outputs for vecNs or matNs respectively.
   *
   * @param from The glm::vecN or glm::matN to be converted.
   */
  static std::optional<std::string> convert(const TFrom& from) {
    return glm::to_string(from);
  }
};

/**
 * @brief Converts from a std::string_view to a std::string.
 */
template <> struct MetadataConversions<std::string, std::string_view> {
  /**
   * @brief Converts from a std::string_view to a std::string.
   */
  static std::optional<std::string> convert(const std::string_view& from) {
    return std::string(from.data(), from.size());
  }
};

#pragma endregion

#pragma region Conversions to glm::vecN
/**
 * @brief Converts from a boolean to a vecN.
 */
template <typename TTo>
struct MetadataConversions<
    TTo,
    bool,
    std::enable_if_t<IsMetadataVecN<TTo>::value>> {
  /**
   * @brief Converts a boolean to a vecN. The boolean is converted to an integer
   * value of 1 for true or 0 for false. The returned vector is initialized with
   * this value in both of its components.
   *
   * @param from The boolean to be converted.
   */
  static std::optional<TTo> convert(bool from) {
    return from ? TTo(1) : TTo(0);
  }
};

/**
 * @brief Converts from a scalar type to a vecN.
 */
template <typename TTo, typename TFrom>
struct MetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataVecN<TTo>::value &&
        CesiumGltf::IsMetadataScalar<TFrom>::value>> {
  /**
   * @brief Converts a scalar to a vecN. The returned vector is initialized
   * with the value in all of its components. The value may lose precision
   * during conversion depending on the type of the scalar and the component
   * type of the matrix.
   *
   * If the scalar cannot be reasonably converted to the component type of the
   * vecN, std::nullopt is returned.
   *
   * @param from The scalar value to be converted.
   */
  static std::optional<TTo> convert(TFrom from) {
    using ToValueType = typename TTo::value_type;

    std::optional<ToValueType> maybeValue =
        MetadataConversions<ToValueType, TFrom>::convert(from);
    if (maybeValue) {
      ToValueType value = *maybeValue;
      return TTo(value);
    }

    return std::nullopt;
  }
};

/**
 * @brief Converts from a vecN type to another vecN type.
 */
template <typename TTo, typename TFrom>
struct MetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataVecN<TTo>::value &&
        CesiumGltf::IsMetadataVecN<TFrom>::value &&
        !std::is_same_v<TTo, TFrom>>> {
  /**
   * @brief Converts a value of the given vecN to another vecN type.
   *
   * If the given vector has more components than the target vecN type, then
   * only its first N components will be used, where N is the dimension of the
   * target vecN type. Otherwise, if the target vecN type has more components,
   * its extra components will be initialized to zero.
   *
   * If any of the relevant components cannot be converted to the target vecN
   * component type, std::nullopt is returned.
   *
   * @param from The vecN value to convert from.
   */
  static std::optional<TTo> convert(TFrom from) {
    TTo result = TTo(0);

    constexpr glm::length_t validLength =
        glm::min(TTo::length(), TFrom::length());

    using ToValueType = typename TTo::value_type;
    using FromValueType = typename TFrom::value_type;

    for (glm::length_t i = 0; i < validLength; i++) {
      auto maybeValue =
          MetadataConversions<ToValueType, FromValueType>::convert(from[i]);
      if (!maybeValue) {
        return std::nullopt;
      }

      result[i] = *maybeValue;
    }

    return result;
  }
};
#pragma endregion

#pragma region Conversions to glm::matN
/**
 * @brief Converts from a boolean to a matN.
 */
template <typename TTo>
struct MetadataConversions<
    TTo,
    bool,
    std::enable_if_t<IsMetadataMatN<TTo>::value>> {
  /**
   * @brief Converts a boolean to a matN. The boolean is converted to an integer
   * value of 1 for true or 0 for false. The returned matrix is initialized with
   * this value in all of its components.
   *
   * @param from The boolean to be converted.
   */
  static std::optional<TTo> convert(bool from) {
    return from ? TTo(1) : TTo(0);
  }
};

/**
 * Converts from a scalar type to a matN.
 */
template <typename TTo, typename TFrom>
struct MetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataMatN<TTo>::value &&
        CesiumGltf::IsMetadataScalar<TFrom>::value>> {
  /**
   * Converts a scalar to a matN. The returned vector is initialized
   * with the value in all components. The value may lose precision during
   * conversion depending on the type of the scalar and the component type of
   * the matrix.
   *
   * If the scalar cannot be reasonably converted to the component type of the
   * matN, std::nullopt is returned.
   *
   * @param from The scalar value to be converted.
   */
  static std::optional<TTo> convert(TFrom from) {
    using ToValueType = typename TTo::value_type;

    std::optional<ToValueType> maybeValue =
        MetadataConversions<ToValueType, TFrom>::convert(from);
    if (!maybeValue) {
      return std::nullopt;
    }

    ToValueType value = *maybeValue;
    return TTo(value);
  }
};

/**
 * @brief Converts from a matN type to another matN type.
 */
template <typename TTo, typename TFrom>
struct MetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataMatN<TTo>::value &&
        CesiumGltf::IsMetadataMatN<TFrom>::value &&
        !std::is_same_v<TTo, TFrom>>> {
  /**
   * @brief Converts a value of the given matN to another matN type.
   *
   * Let M be the length of the given matN, and N be the length of the target
   * matN. If M > N, then only the first N components of the first N columns
   * will be used. Otherwise, if M < N, all other elements in the N x N matrix
   * will be initialized to zero.
   *
   * If any of the relevant components cannot be converted to the target matN
   * component type, std::nullopt is returned.
   *
   * @param from The matN value to convert from.
   */
  static std::optional<TTo> convert(TFrom from) {
    TTo result = TTo(0);

    constexpr glm::length_t validLength =
        glm::min(TTo::length(), TFrom::length());

    using ToValueType = typename TTo::value_type;
    using FromValueType = typename TFrom::value_type;

    for (glm::length_t c = 0; c < validLength; c++) {
      for (glm::length_t r = 0; r < validLength; r++) {
        auto maybeValue =
            MetadataConversions<ToValueType, FromValueType>::convert(
                from[c][r]);
        if (!maybeValue) {
          return std::nullopt;
        }

        result[c][r] = *maybeValue;
      }
    }

    return result;
  }
};
#pragma endregion

} // namespace CesiumGltf
