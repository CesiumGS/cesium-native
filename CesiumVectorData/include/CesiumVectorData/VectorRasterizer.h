#pragma once

#include "VectorStyle.h"

#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/Color.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumVectorData/GeoJsonObject.h>

#include <blend2d.h>
#include <blend2d/context.h>
#include <blend2d/image.h>

#include <span>

namespace CesiumVectorData {

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
   * CesiumGltf::ImageAsset must be four channels, with
   * only one byte per channel (RGBA32).
   * @param mipLevel The mip level that the rasterizer should rasterize for the
   * image.
   * @param ellipsoid The ellipsoid to use.
   */
  VectorRasterizer(
      const CesiumGeospatial::GlobeRectangle& bounds,
      CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset>& imageAsset,
      uint32_t mipLevel = 0,
      const CesiumGeospatial::Ellipsoid& ellipsoid =
          CesiumGeospatial::Ellipsoid::WGS84);

  /**
   * @brief Draws a \ref CesiumGeospatial::CartographicPolygon to the canvas.
   *
   * @param polygon The polygon to draw.
   * @param style The \ref PolygonStyle to use when drawing the polygon.
   */
  void drawPolygon(
      const CesiumGeospatial::CartographicPolygon& polygon,
      const PolygonStyle& style);

  /**
   * @brief Draws a set of linear rings representing a polygon and its holes to
   * the canvas.
   *
   * @param polygon The polygon to draw. It is assumed to have right-hand
   * winding order (exterior rings are counterclockwise, holes are clockwise) as
   * is the case in GeoJSON. The coordinates should be specified in degrees.
   * @param style The \ref PolygonStyle to use when drawing the polygon.
   */
  void drawPolygon(
      const std::vector<std::vector<glm::dvec3>>& polygon,
      const PolygonStyle& style);

  /**
   * @brief Draws a polyline (a set of multiple line segments) to the canvas.
   *
   * @param points The set of points making up the polyline. The coordinates
   * should be specified in degrees.
   * @param style The \ref LineStyle to use when drawing the polyline.
   */
  void
  drawPolyline(const std::vector<glm::dvec3>& points, const LineStyle& style);

  /**
   * @brief Rasterizes a `GeoJsonObject` to the canvas.
   *
   * This will recurse through any children of the `GeoJsonObject` as well. All
   * GeoJSON objects will be *considered* (that is, no object's children will be
   * ignored), but only `LineString` types (`LineString` and `MultiLineString`)
   * and `Polygon` types (`Polygon` and `MultiPolygon`) will actually be
   * rendered.
   *
   * This method can potentially be very slow if a large tree is passed in. If
   * better performance is needed, selecting a subset of leaf objects (those
   * without any children) and calling `drawGeoJsonObject` on each one will have
   * better results.
   *
   * @param geoJsonObject The GeoJSON object to draw.
   * @param style The \ref VectorStyle to use when drawing the object.
   */
  void drawGeoJsonObject(
      const GeoJsonObject& geoJsonObject,
      const VectorStyle& style);

  /**
   * @brief Fills the entire canvas with the given color.
   *
   * @param clearColor The color to use to clear the canvas.
   */
  void clear(const CesiumUtility::Color& clearColor);

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
  uint32_t _mipLevel;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  bool _finalized = false;
};
} // namespace CesiumVectorData