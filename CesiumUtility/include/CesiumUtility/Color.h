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
   * @param r The red component.
   * @param g The green component.
   * @param b The blue component.
   * @param a The alpha component.
   */
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff);

  /**
   * @brief Converts this color to a packed 32-bit number in the form
   * 0xAARRGGBB.
   */
  uint32_t toRgba32() const;
};
} // namespace CesiumUtility