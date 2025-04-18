#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Assert.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <experimental/simd>

#include <algorithm>
#include <cstddef>

using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumGltf;

namespace CesiumVectorData {
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
 * @brief Computes the orientation of point p relative to the edge v0v1.
 *
 * If this value is positive, the point is to the left of the line (from
 * the perspective of standing on point v0 looking at point v1). If it's
 * negative, the point is to the right from the same perspective. If it's zero,
 * the point is on the edge.
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

  const size_t imageX = (size_t)((image.width - 1) * normalizedX);
  const size_t imageY = (size_t)((image.width - 1) * normalizedY);
  const size_t baseIndex =
      (imageY * (size_t)image.height + imageX) * (size_t)image.channels;
  if (image.channels == 1) {
    image.pixelData[baseIndex] = color[0];
  } else if (image.channels == 2) {
    image.pixelData[baseIndex] = color[0];
    image.pixelData[baseIndex + 1] = color[1];
  } else if (image.channels == 3) {
    image.pixelData[baseIndex] = color[0];
    image.pixelData[baseIndex + 1] = color[1];
    image.pixelData[baseIndex + 2] = color[2];
  } else if (image.channels == 4) {
    if (color[3] == std::byte{255}) {
      // New color is opaque, no need to blend
      image.pixelData[baseIndex] = color[0];
      image.pixelData[baseIndex + 1] = color[1];
      image.pixelData[baseIndex + 2] = color[2];
      image.pixelData[baseIndex + 3] = color[3];
    } else {
      // Alpha blending time!
      const float r1 = (float)image.pixelData[baseIndex] / 255.0f;
      const float g1 = (float)image.pixelData[baseIndex + 1] / 255.0f;
      const float b1 = (float)image.pixelData[baseIndex + 2] / 255.0f;
      const float a1 = (float)image.pixelData[baseIndex + 3] / 255.0f;
      const float r0 = (float)color[0] / 255.0f;
      const float g0 = (float)color[1] / 255.0f;
      const float b0 = (float)color[2] / 255.0f;
      const float a0 = (float)color[3] / 255.0f;

      image.pixelData[baseIndex] =
          (std::byte)((r0 * a0 + r1 * a1 * (1.0f - a0)) * 255.0f);
      image.pixelData[baseIndex + 1] =
          (std::byte)((g0 * a0 + g1 * a1 * (1.0f - a0)) * 255.0f);
      image.pixelData[baseIndex + 2] =
          (std::byte)((b0 * a0 + b1 * a1 * (1.0f - a0)) * 255.0f);
      image.pixelData[baseIndex + 3] =
          (std::byte)((a0 + a1 * (1.0f - a0)) * 255.0f);
    }
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

      glm::dvec2 p{intersection->getWest(), intersection->getSouth()};

      // We can calculate these ahead of time and use them to avoid recomputing
      // the orientation every pixel, on account of knowing that we are always
      // moving one pixel to the right every iteration of the inner loop, and
      // one pixel down every iteration of the outer loop.
      const double x01 = (v1.x - v0.x) * step.x;
      const double x12 = (v2.x - v1.x) * step.x;
      const double x20 = (v0.x - v2.x) * step.x;
      const double y01 = (v0.y - v1.y) * step.y;
      const double y12 = (v1.y - v2.y) * step.y;
      const double y20 = (v2.y - v0.y) * step.y;

      // Calculate orientation at the starting corner.
      double w0Row = edgeOrientation(v1, v2, p);
      double w1Row = edgeOrientation(v2, v0, p);
      double w2Row = edgeOrientation(v0, v1, p);

      for (p.y = intersection->getSouth(); p.y <= intersection->getNorth();
           p.y += step.y) {
        double w0 = w0Row;
        double w1 = w1Row;
        double w2 = w2Row;
        for (p.x = intersection->getWest(); p.x <= intersection->getEast();
             p.x += step.x) {
          if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
            renderPixel(p, polygon.color, rectangle, image);
          }

          w0 += y12;
          w1 += y20;
          w2 += y01;
        }

        w0Row += x12;
        w1Row += x20;
        w2Row += x01;
      }
    }
  }
}
} // namespace CesiumVectorData