#include <CesiumUtility/Color.h>
#include <CesiumVectorData/VectorStyle.h>

#include <cstdint>
#include <optional>
#include <random>

using namespace CesiumUtility;

namespace CesiumVectorData {
Color ColorStyle::getColor() const {
  if (this->colorMode == ColorMode::Normal) {
    return this->color;
  }

  std::random_device r;
  std::mt19937 mt(r());
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);

  return Color{
      (uint8_t)(dist(mt) * (float)this->color.r),
      (uint8_t)(dist(mt) * (float)this->color.g),
      (uint8_t)(dist(mt) * (float)this->color.b),
      this->color.a};
}
VectorStyle::VectorStyle(const CesiumUtility::Color& color)
    : line{{color}}, polygon{ColorStyle{color}, std::nullopt} {}
VectorStyle::VectorStyle(
    const LineStyle& lineStyle,
    const PolygonStyle& polygonStyle)
    : line(lineStyle), polygon(polygonStyle) {}
} // namespace CesiumVectorData