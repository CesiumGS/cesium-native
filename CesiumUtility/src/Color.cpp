#include <CesiumUtility/Color.h>

namespace CesiumUtility {
uint32_t Color::toRgba32() const {
  return this->a << 24 | this->r << 16 | this->g << 8 | this->b;
}
} // namespace CesiumUtility