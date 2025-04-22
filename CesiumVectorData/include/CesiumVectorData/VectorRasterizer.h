#pragma once

#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/CompositeCartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <blend2d/context.h>
#include <blend2d/image.h>

#include <cstddef>
#include <span>

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

/**
 * @brief Rasterizes vector primitives into a \ref CesiumGltf::ImageAsset.
 */
class VectorRasterizer {
public:
  /**
   * @brief Creates a new \ref VectorRasterizer representing the given rectangle
   * on the globe.
   *
   * @param bounds The bounds on the globe that this rasterizer's canvas will
   * cover.
   * @param imageAsset The destination image asset. This \ref
   * CesiumGltf::ImageAsset must be either one channel or four channels, with
   * only one byte per channel (R8 or RGBA32).
   */
  VectorRasterizer(
      const CesiumGeospatial::GlobeRectangle& bounds,
      CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset>& imageAsset);

  /**
   * @brief Draws a \ref CesiumGeospatial::CartographicPolygon to the canvas.
   *
   * @param polygon The polygon to draw.
   * @param drawColor The \ref Color to use to draw the polygon.
   */
  void drawPolygon(
      const CesiumGeospatial::CartographicPolygon& polygon,
      const Color& drawColor);

  /**
   * @brief Draws a \ref CesiumGeospatial::CompositeCartographicPolygon to the
   * canvas.
   *
   * Support for the CompositeCartographicPolygon is limited at the moment.
   * Polygons with holes will rasterize incorrectly.
   *
   * @param polygon The composite polygon to draw.
   * @param drawColor The \ref Color to use to draw the composite polygon.
   */
  void drawPolygon(
      const CesiumGeospatial::CompositeCartographicPolygon& polygon,
      const Color& drawColor);

  /**
   * @brief Draws a polyline (a set of multiple line segments) to the canvas.
   *
   * @param points The set of points making up the polyline.
   * @param drawColor The \ref Color to use to draw the polyline.
   */
  void drawPolyline(
      const std::span<CesiumGeospatial::Cartographic>& points,
      const Color& drawColor);

  /**
   * @brief Finalizes the rasterization operations, flushing all draw calls to
   * the canvas, ensuring proper pixel ordering, and releasing the draw context.
   *
   * Once a \ref VectorRasterizer is finalized, it can no longer be used for
   * drawing. Subsequent calls to its methods will do nothing.
   */
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> finalize();

private:
  CesiumGeospatial::GlobeRectangle _bounds;
  BLImage _image;
  BLContext _context;
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> _imageAsset;
  bool _finalized = false;
};
} // namespace CesiumVectorData