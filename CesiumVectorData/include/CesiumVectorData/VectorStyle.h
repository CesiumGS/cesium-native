#pragma once

#include "Color.h"

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
   * always ignored. For example, if the color was 0xff000077 (only 0x77 in the
   * green component), the resulting randomized value could be 0xff000041, or
   * 0xff000076, but never 0xff0000aa.
   */
  Random = 1
};

/**
 * @brief A base struct used for specifying the color of style types.
 */
struct ColorStyle {
  /** @brief The color to be used. */
  Color color = Color(0xff, 0xff, 0xff, 0xff);
  /** @brief The color mode to be used. */
  ColorMode colorMode = ColorMode::Normal;

  Color getColor() const;
};

/** @brief The style used to draw a Polygon. */
struct PolygonStyle : public ColorStyle {
  /**
   * @brief Whether the polygon should be filled. The `ColorStyle` specified
   * here will be used for the fill color.
   */
  bool fill = true;
  /**
   * @brief Whether the polygon should be outlined. The `LineStyle` specified on
   * the same `VectorStyle` as this will be used for the stroke color.
   */
  bool outline = false;
};

/** @brief Whether the line width is specified in meters or in pixels. */
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

/**
 * @brief Style information to use when drawing vector data.
 */
struct VectorStyle {
  /**
   * @brief Styles to use when drawing polylines and stroking shapes.
   */
  LineStyle line;
  /**
   * @brief Styles to use when drawing polygons.
   */
  PolygonStyle polygon;

  /**
   * @brief Initializes style information for all types.
   */
  VectorStyle(const LineStyle& lineStyle, const PolygonStyle& polygonStyle)
      : line(lineStyle), polygon(polygonStyle) {}

  /**
   * @brief Initializes all styles to the given color.
   */
  VectorStyle(const Color& color) : line{{color}}, polygon{{color}} {}
};
} // namespace CesiumVectorData