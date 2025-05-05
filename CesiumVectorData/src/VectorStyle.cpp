#include <CesiumVectorData/Color.h>
#include <CesiumVectorData/VectorStyle.h>

#include <random>

namespace CesiumVectorData {
Color ColorStyle::getColor() const {
  if (this->colorMode == ColorMode::Normal) {
    return this->color;
  }

  std::random_device r;
  std::mt19937 mt(r());
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);

  return Color{
      std::byte{(uint8_t)(dist(mt) * (float)this->color.r)},
      std::byte{(uint8_t)(dist(mt) * (float)this->color.g)},
      std::byte{(uint8_t)(dist(mt) * (float)this->color.b)},
      this->color.a};
}
} // namespace CesiumVectorData