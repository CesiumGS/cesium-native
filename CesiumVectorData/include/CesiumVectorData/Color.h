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
   * @brief Converts this color to a packed 32-bit number in the form
   * 0xAARRGGBB.
   */
  uint32_t toRgba32() const;
};
} // namespace CesiumVectorData