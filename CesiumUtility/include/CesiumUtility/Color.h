#pragma once

#include <cstdint>

namespace CesiumUtility {
/**
 * @brief Represents an RGBA color value.
 */
struct Color {
  /** @brief The red component. */
  uint8_t r;
  /** @brief The green component. */
  uint8_t g;
  /** @brief The blue component. */
  uint8_t b;
  /** @brief The alpha component. */
  uint8_t a;

  /**
   * @brief Creates a new Color from the given components.
   *
   * @param r_ The red component.
   * @param g_ The green component.
   * @param b_ The blue component.
   * @param a_ The alpha component.
   */
  Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 0xff)
      : r(r_), g(g_), b(b_), a(a_) {}

  /**
   * @brief Converts this color to a packed 32-bit number in the form
   * 0xAARRGGBB.
   */
  uint32_t toRgba32() const;
};
} // namespace CesiumUtility