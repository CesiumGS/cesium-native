#pragma once

#include <CesiumUtility/Color.h>

#include <optional>

namespace CesiumVectorData {

/**
 * @brief The mode used for coloring.
 */
enum class ColorMode : uint8_t {
  /** @brief The normal color mode. The color will be used directly. */
  Normal = 0,
  /**
   * @brief The color will be chosen randomly.
   *
   * The color randomization will be applied to each component, with the
   * resulting value between 0 and the specified color component value. Alpha is
   * always ignored. For example, if the color was `(R: 0x00, G: 0x77, B: 0x00,
   * A: 0xFF)`, the resulting randomized value could be `(R: 0x00, G: 0x41, B:
   * 0x00, A: 0xFF)`, or `(R: 0x00, G: 0x76, B: 0x00, A: 0xFF)`, but never `(R:
   * 0x00, G: 0xAA, B: 0x00, A: 0xFF)`.
   */
  Random = 1
};

/**
 * @brief Specifies the color of a style type.
 */
struct ColorStyle {
  /** @brief The color to be used. */
  CesiumUtility::Color color = CesiumUtility::Color(0xff, 0xff, 0xff, 0xff);
  /** @brief The color mode to be used. */
  ColorMode colorMode = ColorMode::Normal;

  /**
   * @brief Obtains the color specified on this `ColorStyle`.
   *
   * For `ColorMode::Normal`, this just returns the value of `color`. For
   * `ColorMode::Random`, this returns a randomized value obtained based on
   * the rules described in \ref ColorMode.
   *
   * @param randomColorSeed The seed for the random color to be generated, if
   * `colorMode` is set to `Random`. The same color will always be returned for
   * a given seed, but nearby seeds will not usually return nearby colors.
   */
  CesiumUtility::Color getColor(size_t randomColorSeed = 0) const;
};

/** @brief The mode to use when interpreting a given line width. */
enum class LineWidthMode : uint8_t {
  /**
   * @brief The line will always be this number of pixels in width, no matter
   * how close the user gets to the line.
   */
  Pixels = 0,
  /**
   * @brief The line width will cover this number of meters of the ellipsoid
   * it's rendered on. This may cause the line to disappear as the user zooms
   * out.
   *
   * This value specifies a size in meters *at the equator* of the ellipsoid
   * it's rendered on.
   */
  Meters = 1
};

/** @brief The style used to draw polylines and strokes. */
struct LineStyle : public ColorStyle {
  /**
   * @brief The width of this line. If `widthMode` is set to
   * `LineWidthMode::Pixels`, this is in pixels. Otherwise, if set to
   * `LineWidthMode::Meters`, it is in meters.
   */
  double width = 1.0;
  /**
   * @brief The mode to use when interpreting `width`.
   */
  LineWidthMode widthMode = LineWidthMode::Pixels;
};

/** @brief The style used to draw a Polygon. */
struct PolygonStyle {
  /**
   * @brief The color used to fill this polygon. If `std::nullopt`, the polygon
   * will not be filled.
   */
  std::optional<ColorStyle> fill;
  /**
   * @brief The style used to outline this polygon. If `std::nullopt`, the
   * polygon will not be outlined.
   */
  std::optional<LineStyle> outline;
};

/**
 * @brief Style information to use when drawing vector data.
 */
struct VectorStyle {
  /**
   * @brief The style to use when drawing polylines.
   */
  LineStyle line;
  /**
   * @brief The style to use when drawing polygons.
   */
  PolygonStyle polygon;

  /**
   * @brief Default constructor for VectorStyle.
   */
  VectorStyle() = default;

  /**
   * @brief Initializes style information for all types.
   */
  VectorStyle(const LineStyle& lineStyle, const PolygonStyle& polygonStyle);

  /**
   * @brief Initializes all styles to the given color.
   */
  VectorStyle(const CesiumUtility::Color& color);
};
} // namespace CesiumVectorData