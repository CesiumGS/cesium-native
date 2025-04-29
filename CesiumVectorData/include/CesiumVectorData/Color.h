#pragma once

#include <cstddef>
#include <cstdint>

namespace CesiumVectorData {
/**
 * @brief Represents an RGBA color value.
 */
struct Color {
  /** @brief The red component. */
  std::byte r;
  /** @brief The green component. */
  std::byte g;
  /** @brief The blue component. */
  std::byte b;
  /** @brief The alpha component. */
  std::byte a;

  /**
   * @brief Creates a new Color from the given components.
   *
   * @param r_ The red component.
   * @param g_ The green component.
   * @param b_ The blue component.
   * @param a_ The alpha component.
   */
  Color(
      std::byte r_,
      std::byte g_,
      std::byte b_,
      std::byte a_ = std::byte{0xff})
      : r(r_), g(g_), b(b_), a(a_) {}

  /**
   * @brief Creates a new Color from the given components.
   *
   * @param r_ The red component.
   * @param g_ The green component.
   * @param b_ The blue component.
   * @param a_ The alpha component.
   */
  Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 0xff)
      : r(std::byte{r_}),
        g(std::byte{g_}),
        b(std::byte{b_}),
        a(std::byte{a_}) {}

  /**
   * @brief Converts this color to a packed 32-bit number in the form
   * 0xAARRGGBB.
   */
  uint32_t toRgba32() const;
};
} // namespace CesiumVectorData