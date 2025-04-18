#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Assert.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <glm/vec2.hpp>
#include <xsimd/xsimd.hpp>

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
/*inline float
edgeOrientation(const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& p) {
  return (v1.x - v0.x) * (p.y - v0.y) - (v1.y - v0.y) * (p.x - v0.x);
}*/

struct Edge {
  static const int stepXSize = 4;
  static const int stepYSize = 1;

  xsimd::batch<float> oneStepX;
  xsimd::batch<float> oneStepY;

  xsimd::batch<float> init(
      const glm::vec2& v0,
      const glm::vec2& v1,
      const glm::vec2& p,
      const glm::vec2& step) {
    float a = v0.y - v1.y;
    float b = v1.x - v0.x;
    float c = v0.x * v1.y - v0.y * v1.x;

    oneStepX = a * (float)stepXSize * step.x;
    oneStepY = b * (float)stepYSize * step.y;

    xsimd::batch<float> x =
        p.x + xsimd::batch<float>{0, step.x, 2 * step.x, 3 * step.x};
    xsimd::batch<float> y = p.y;

    return x * a + y * b + c;
  }
};

void renderPixel(
    size_t imageX,
    size_t imageY,
    const std::array<std::byte, 4>& color,
    ImageAsset& image) {
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
  const glm::vec2 step{
      rectangle.computeWidth() / (double)image.width,
      rectangle.computeHeight() / (double)image.height};
  const glm::vec2 vectorizedStep = step * glm::vec2(Edge::stepXSize, Edge::stepYSize);
  bool arr[4] { false, false, false, false };

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

      // Though we're stepping through by spatial coordinates, calculating the
      // pixel position of minX and minY lets us avoid doing the normalization
      // step every time we draw a pixel, instead doing it only once per
      // triangle.
      const double normalizedX =
          (intersection->getWest() - rectangle.getWest()) /
          rectangle.computeWidth();
      const double normalizedY =
          (intersection->getSouth() - rectangle.getSouth()) /
          rectangle.computeHeight();
      CESIUM_ASSERT(normalizedX >= 0.0 && normalizedX <= 1.0);
      CESIUM_ASSERT(normalizedY >= 0.0 && normalizedY <= 1.0);

      size_t imageX = (size_t)((image.width - 1) * normalizedX);
      const size_t baseImageX = imageX;
      size_t imageY = (size_t)((image.width - 1) * normalizedY);

      // Lookup the vertices for this triangle. Since this is a CW-wound
      // triangle, we need to swap v1 and v2 to maintain our "left of the line =
      // inside" rule.
      const glm::dvec2 base =
          glm::dvec2(intersection->getWest(), intersection->getSouth());
      const float width = (float)intersection->computeWidth();
      const float height = (float)intersection->computeHeight();
      const glm::vec2 v0 =
          (glm::vec2)(polygon.vertices[polygon.indices[i * 3]] - base);
      const glm::vec2 v1 =
          (glm::vec2)(polygon.vertices[polygon.indices[i * 3 + 1]] - base);
      const glm::vec2 v2 =
          (glm::vec2)(polygon.vertices[polygon.indices[i * 3 + 2]] - base);

      glm::vec2 p{0, 0};

      Edge e01, e12, e20;

      xsimd::batch<float> w0Row = e12.init(v1, v2, p, step);
      xsimd::batch<float> w1Row = e20.init(v2, v0, p, step);
      xsimd::batch<float> w2Row = e01.init(v0, v1, p, step);

      for (p.y = 0; p.y <= height; p.y += vectorizedStep.y) {
        xsimd::batch<float> w0 = w0Row;
        xsimd::batch<float> w1 = w1Row;
        xsimd::batch<float> w2 = w2Row;
        for (p.x = 0; p.x <= width; p.x += vectorizedStep.x) {
          xsimd::batch_bool<float> mask = w0 >= 0 && w1 >= 0 && w2 >= 0;
          if (xsimd::any(mask)) {
            mask.store_unaligned(arr);
            if(arr[0]) {
              renderPixel(imageX, imageY, polygon.color, image);
            }
            if(arr[1]) {
              renderPixel(imageX + 1, imageY, polygon.color, image);
            }
            if(arr[2]) {
              renderPixel(imageX + 2, imageY, polygon.color, image);
            }
            if(arr[3]) {
              renderPixel(imageX + 3, imageY, polygon.color, image);
            }
          }

          w0 += e12.oneStepX;
          w1 += e20.oneStepX;
          w2 += e01.oneStepX;
          imageX += Edge::stepXSize;
        }

        w0Row += e12.oneStepY;
        w1Row += e20.oneStepY;
        w2Row += e01.oneStepY;
        imageX = baseImageX;
        imageY += Edge::stepYSize;
      }
    }
  }
}
} // namespace CesiumVectorData
