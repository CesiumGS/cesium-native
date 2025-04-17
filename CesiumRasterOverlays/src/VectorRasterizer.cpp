#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumRasterOverlays/VectorRasterizer.h>
#include <CesiumUtility/Assert.h>

#include <experimental/simd>

#include <algorithm>
#include <cstddef>

using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumGltf;

namespace CesiumRasterOverlays {
VectorRasterizer::VectorRasterizer(
    const std::vector<CartographicPolygon>& primitives,
    const std::vector<std::array<std::byte, 4>>& colors) {
  CESIUM_ASSERT(primitives.size() == colors.size());
  // Only polygon primitives right now so the sizes are equivalent.
  this->_polygons.reserve(primitives.size());

  for (size_t i = 0; i < primitives.size(); i++) {
    if (primitives[i].getIndices().size() < 3) {
      // Polygon contains no triangles - ignore.
      return;
    }

    const GlobeRectangle& globeRect = primitives[i].getBoundingRectangle();
    std::vector<GlobeRectangle> triangleRects;
    triangleRects.reserve(primitives[i].getIndices().size() / 3);
    for (size_t j = 0; j < primitives[i].getIndices().size(); j += 3) {
      const glm::dvec2& a = primitives[i].getVertices()[j];
      const glm::dvec2& b = primitives[i].getVertices()[j + 1];
      const glm::dvec2& c = primitives[i].getVertices()[j + 2];

      triangleRects.emplace_back(
          std::min({a.x, b.x, c.x}),
          std::min({a.y, b.y, c.y}),
          std::max({a.x, b.x, c.x}),
          std::max({a.y, b.y, c.y}));
    }

    const Cartographic& origin = globeRect.getSouthwest();

    this->_polygons.emplace_back(PolygonData{
        globeRect,
        origin,
        std::move(triangleRects),
        primitives[i].getVertices(),
        primitives[i].getIndices(),
        colors[i]});
  }
}

/**
 * For more information on the following code, take a look at Fabian Giesen's
 * "Optimizing Software Occlusion Culling" articles:
 * https://fgiesen.wordpress.com/2013/02/17/optimizing-sw-occlusion-culling-index/
 *
 * (Despite the title, this series deals a lot with triangle rasterization.)
 */

namespace {
/**
 * @brief Computes the edge determinant of the edge v0v1 and the point p.
 *
 * If this determinant is positive, the point is to the left of the line (from
 * the perspective of standing on point v0 looking at point v1).
 *
 * If a point is to the left of all three edges of a triangle with CCW winding,
 * it is in the triangle. The triangles earcut produces are CW, so we need to
 * swap the vertices to make this work, but otherwise the principle is the same.
 */
inline double edgeOrientation(
    const glm::dvec2& v0,
    const glm::dvec2& v1,
    const glm::dvec2& p) {
  return (v1.x - v0.x) * (p.y - v0.y) - (v1.y - v0.y) * (p.x - v0.x);
}

inline void renderPixel(
    const glm::dvec2& point,
    const std::array<std::byte, 4>& color,
    const GlobeRectangle& rect,
    ImageAsset& image) {
  double normalizedX = (point.x - rect.getWest()) / rect.computeWidth();
  double normalizedY = (point.y - rect.getSouth()) / rect.computeHeight();
  CESIUM_ASSERT(normalizedX >= 0.0 && normalizedX <= 1.0);
  CESIUM_ASSERT(normalizedY >= 0.0 && normalizedY <= 1.0);

  size_t imageX = (size_t)((image.width - 1) * normalizedX);
  size_t imageY = (size_t)((image.width - 1) * normalizedY);
  if (image.channels == 1) {
    image.pixelData[imageY * (size_t)image.height + imageX] = color[0];
  } else if (image.channels == 2) {
    image.pixelData[imageY * 2 * (size_t)image.height + imageX * 2] = color[0];
    image.pixelData[imageY * 2 * (size_t)image.height + imageX * 2 + 1] =
        color[1];
  } else if (image.channels == 3) {
    image.pixelData[imageY * 3 * (size_t)image.height + imageX * 3] = color[0];
    image.pixelData[imageY * 3 * (size_t)image.height + imageX * 3 + 1] =
        color[1];
    image.pixelData[imageY * 3 * (size_t)image.height + imageX * 3 + 2] =
        color[2];
  } else if (image.channels == 4) {
    image.pixelData[imageY * 4 * (size_t)image.height + imageX * 4] = color[0];
    image.pixelData[imageY * 4 * (size_t)image.height + imageX * 4 + 1] =
        color[1];
    image.pixelData[imageY * 4 * (size_t)image.height + imageX * 4 + 2] =
        color[2];
    image.pixelData[imageY * 4 * (size_t)image.height + imageX * 4 + 3] =
        color[3];
  }
}
} // namespace

void VectorRasterizer::rasterize(
    const GlobeRectangle& rectangle,
    ImageAsset& image) {
  glm::dvec2 step{
      rectangle.computeWidth() / (double)image.width,
      rectangle.computeHeight() / (double)image.height};

  for (const PolygonData& polygon : this->_polygons) {
    if (!rectangle.computeIntersection(polygon.boundingRectangle)) {
      // Polygon not visible within rectangle.
      continue;
    }

    // TODO: optimization when rectangle is entirely in polygon

    for (size_t i = 0; i < polygon.triangleBoundingRectangles.size(); i++) {
      const std::optional<GlobeRectangle> intersection =
          polygon.triangleBoundingRectangles[i].computeIntersection(rectangle);
      if (!intersection) {
        // Triangle not visible.
        continue;
      }

      // Lookup the vertices for this triangle. Since this is a CW-wound
      // triangle, we need to swap v1 and v2 to maintain our "left of the line =
      // inside" rule.
      const glm::dvec2& v0 = polygon.vertices[polygon.indices[i * 3]];
      const glm::dvec2& v1 = polygon.vertices[polygon.indices[i * 3 + 1]];
      const glm::dvec2& v2 = polygon.vertices[polygon.indices[i * 3 + 2]];

      glm::dvec2 p;
      for (p.y = intersection->getSouth(); p.y <= intersection->getNorth();
           p.y += step.y) {
        for (p.x = intersection->getWest(); p.x <= intersection->getEast();
             p.x += step.x) {
          double w0 = edgeOrientation(v1, v2, p);
          double w1 = edgeOrientation(v2, v0, p);
          double w2 = edgeOrientation(v0, v1, p);
          if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
            renderPixel(p, polygon.color, rectangle, image);
          }
        }
      }
    }
  }
}
} // namespace CesiumRasterOverlays