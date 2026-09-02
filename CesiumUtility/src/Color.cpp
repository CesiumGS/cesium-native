#include <CesiumUtility/Color.h>

#include <cstdint>

namespace CesiumUtility {
uint32_t Color::toRgba32() const {
  return (uint32_t)this->a << 24 | (uint32_t)this->r << 16 |
         (uint32_t)this->g << 8 | this->b;
}

Color::Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_)
    : r(r_), g(g_), b(b_), a(a_) {}
bool Color::operator==(const Color& rhs) const {
  return this->r == rhs.r && this->g == rhs.g && this->b == rhs.b &&
         this->a == rhs.a;
}
} // namespace CesiumUtility