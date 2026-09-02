#include <CesiumUtility/Color.h>
#include <CesiumUtility/Hash.h>
#include <CesiumVectorData/VectorStyle.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

using namespace CesiumUtility;

namespace CesiumVectorData {
Color ColorStyle::getColor(size_t randomColorSeed) const {
  if (this->colorMode == ColorMode::Normal) {
    return this->color;
  }

  std::hash<size_t> hash{};
  size_t h = hash(randomColorSeed);

  float r = uint8_t(h & 0xFF) / 255.0f;
  float g = uint8_t((h >> 8) & 0xFF) / 255.0f;
  float b = uint8_t((h >> 16) & 0xFF) / 255.0f;

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
VectorStyle::VectorStyle(
    const LineStyle& lineStyle,
    const PolygonStyle& polygonStyle,
    const PointStyle& pointStyle)
    : line(lineStyle), polygon(polygonStyle), point(pointStyle) {}

bool ColorStyle::operator==(const ColorStyle& rhs) const {
  return this->color == rhs.color && this->colorMode == rhs.colorMode;
}

bool LineStyle::operator==(const LineStyle& rhs) const {
  return this->width == rhs.width && this->widthMode == rhs.widthMode &&
         this->color == rhs.color && this->colorMode == rhs.colorMode;
}

bool PolygonStyle::operator==(const PolygonStyle& rhs) const {
  return this->fill == rhs.fill && this->outline == rhs.outline;
}

bool PointStyle::operator==(const PointStyle& rhs) const {
  return this->radius == rhs.radius && this->fill == rhs.fill &&
         this->outline == rhs.outline;
}
bool VectorStyle::operator==(const VectorStyle& rhs) const {
  return this->line == rhs.line && this->polygon == rhs.polygon &&
         this->point == rhs.point;
}
} // namespace CesiumVectorData

std::size_t std::hash<CesiumVectorData::ColorStyle>::operator()(
    const CesiumVectorData::ColorStyle& style) const noexcept {
  std::size_t result = std::hash<uint32_t>{}(style.color.toRgba32());
  result = CesiumUtility::Hash::combine(
      result,
      std::hash<uint8_t>{}(static_cast<uint8_t>(style.colorMode)));
  return result;
}

std::size_t std::hash<CesiumVectorData::LineStyle>::operator()(
    const CesiumVectorData::LineStyle& style) const noexcept {
  std::size_t result = std::hash<CesiumVectorData::ColorStyle>{}(style);
  result =
      CesiumUtility::Hash::combine(result, std::hash<double>{}(style.width));
  result = CesiumUtility::Hash::combine(
      result,
      std::hash<uint8_t>{}(static_cast<uint8_t>(style.widthMode)));
  return result;
}

std::size_t std::hash<CesiumVectorData::PolygonStyle>::operator()(
    const CesiumVectorData::PolygonStyle& style) const noexcept {
  std::size_t result =
      std::hash<std::optional<CesiumVectorData::ColorStyle>>{}(style.fill);
  result = CesiumUtility::Hash::combine(
      result,
      std::hash<std::optional<CesiumVectorData::LineStyle>>{}(style.outline));
  return result;
}

std::size_t std::hash<CesiumVectorData::PointStyle>::operator()(
    const CesiumVectorData::PointStyle& style) const noexcept {
  std::size_t result =
      std::hash<std::optional<CesiumVectorData::ColorStyle>>{}(style.fill);
  result = CesiumUtility::Hash::combine(
      result,
      std::hash<std::optional<CesiumVectorData::LineStyle>>{}(style.outline));
  result =
      CesiumUtility::Hash::combine(result, std::hash<double>{}(style.radius));
  return result;
}

std::size_t std::hash<CesiumVectorData::VectorStyle>::operator()(
    const CesiumVectorData::VectorStyle& style) const noexcept {
  std::size_t result = std::hash<CesiumVectorData::LineStyle>{}(style.line);
  result = CesiumUtility::Hash::combine(
      result,
      std::hash<CesiumVectorData::PolygonStyle>{}(style.polygon));
  result = CesiumUtility::Hash::combine(
      result,
      std::hash<CesiumVectorData::PointStyle>{}(style.point));
  return result;
}
