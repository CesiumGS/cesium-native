#include <CesiumUtility/Color.h>
#include <CesiumVectorData/VectorStyle.h>

#include <cstdint>
#include <optional>
#include <utility>

using namespace CesiumUtility;

namespace CesiumVectorData {
Color ColorStyle::getColor(size_t randomColorSeed) const {
  if (this->colorMode == ColorMode::Normal) {
    return this->color;
  }

  std::hash<size_t> hash{};
  size_t h = hash(randomColorSeed);

  float r = (h & 0xFF) / 255.0f;
  float g = ((h >> 8) & 0xFF) / 255.0f;
  float b = ((h >> 16) & 0xFF) / 255.0f;

  return Color{
      (uint8_t)(r * this->color.r),
      (uint8_t)(g * this->color.g),
      (uint8_t)(b * this->color.b),
      this->color.a};
}
VectorStyle::VectorStyle(const CesiumUtility::Color& color)
    : line{{color}}, polygon{ColorStyle{color}, std::nullopt} {}
VectorStyle::VectorStyle(
    const LineStyle& lineStyle,
    const PolygonStyle& polygonStyle)
    : line(lineStyle), polygon(polygonStyle) {}
} // namespace CesiumVectorData