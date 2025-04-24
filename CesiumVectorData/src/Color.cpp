#include <CesiumVectorData/Color.h>

namespace CesiumVectorData {
uint32_t Color::toRgba32() const {
  return (uint32_t)this->a << 24 | (uint32_t)this->r << 16 |
         (uint32_t)this->g << 8 | (uint32_t)this->b;
}
} // namespace CesiumVectorData