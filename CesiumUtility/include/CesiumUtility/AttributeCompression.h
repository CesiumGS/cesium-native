#pragma once

#include <CesiumUtility/Library.h>
#include <CesiumUtility/Math.h>

#include <glm/glm.hpp>

namespace CesiumUtility {
/**
 * @brief Functions to handle compressed attributes in different formats
 */
class CESIUMUTILITY_API AttributeCompression final {

public:
  /**
   * @brief Decodes a unit-length vector in 'oct' encoding to a normalized
   * 3-component vector.
   *
   * @param x The x component of the oct-encoded unit length vector.
   * @param y The y component of the oct-encoded unit length vector.
   * @param rangeMax The maximum value of the SNORM range. The encoded vector is
   * stored in log2(rangeMax+1) bits.
   * @returns The decoded and normalized vector.
   */
  template <
      typename T,
      class = typename std::enable_if<std::is_unsigned<T>::value>::type>
  static glm::dvec3 octDecodeInRange(T x, T y, T rangeMax) {
    glm::dvec3 result;

    result.x = CesiumUtility::Math::fromSNorm(x, rangeMax);
    result.y = CesiumUtility::Math::fromSNorm(y, rangeMax);
    result.z = 1.0 - (glm::abs(result.x) + glm::abs(result.y));

    if (result.z < 0.0) {
      const double oldVX = result.x;
      result.x =
          (1.0 - glm::abs(result.y)) * CesiumUtility::Math::signNotZero(oldVX);
      result.y =
          (1.0 - glm::abs(oldVX)) * CesiumUtility::Math::signNotZero(result.y);
    }

    return glm::normalize(result);
  }

  /**
   * @brief Decodes a unit-length vector in 2 byte 'oct' encoding to a
   * normalized 3-component vector.
   *
   * @param x The x component of the oct-encoded unit length vector.
   * @param y The y component of the oct-encoded unit length vector.
   * @returns The decoded and normalized vector.
   *
   * @see AttributeCompression::octDecodeInRange
   */
  static glm::dvec3 octDecode(uint8_t x, uint8_t y) {
    constexpr uint8_t rangeMax = 255;
    return AttributeCompression::octDecodeInRange(x, y, rangeMax);
  }

  /**
   * @brief Decodes a RGB565-encoded color to a 3-component vector
   * containing the normalized RGB values.
   *
   * @param value The RGB565-encoded value.
   * @returns The normalized RGB values.
   */
  static glm::dvec3 decodeRGB565(const uint16_t value) {
    constexpr uint16_t mask5 = (1 << 5) - 1;
    constexpr uint16_t mask6 = (1 << 6) - 1;
    constexpr float normalize5 = 1.0f / 31.0f; // normalize [0, 31] to [0, 1]
    constexpr float normalize6 = 1.0f / 63.0f; // normalize [0, 63] to [0, 1]

    const uint16_t red = static_cast<uint16_t>(value >> 11);
    const uint16_t green = static_cast<uint16_t>((value >> 5) & mask6);
    const uint16_t blue = value & mask5;

    return glm::dvec3(red * normalize5, green * normalize6, blue * normalize5);
  };
};

} // namespace CesiumUtility
