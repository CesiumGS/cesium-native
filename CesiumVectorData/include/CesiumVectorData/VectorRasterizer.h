#pragma once

#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/CompositeCartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>

#include <glm/ext/vector_double2.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace CesiumVectorData {

struct PolygonData {
  CesiumGeospatial::GlobeRectangle boundingRectangle;
  CesiumGeospatial::Cartographic origin;
  std::vector<CesiumGeospatial::GlobeRectangle> triangleBoundingRectangles;
  std::vector<glm::dvec2> vertices;
  std::vector<uint32_t> indices;
  std::array<std::byte, 4> color;
};

using RasterizationPrimitive = std::variant<
    CesiumGeospatial::CartographicPolygon,
    CesiumGeospatial::CompositeCartographicPolygon>;

class VectorRasterizer {
public:
  VectorRasterizer(
      const std::vector<CesiumGeospatial::CartographicPolygon>& primitives,
      const std::vector<std::array<std::byte, 4>>& colors);

  /**
   * @brief Rasterizes the primitives provided to this rasterizer over the top
   * of the given image.
   *
   * @param rectangle The \ref CesiumGeospatial::GlobeRectangle that this image
   * covers.
   * @param destinationImage The image that the vector primitives will be
   * rasterized to.
   */
  void rasterize(
      const CesiumGeospatial::GlobeRectangle& rectangle,
      CesiumGltf::ImageAsset& destinationImage);

private:
  std::vector<PolygonData> _polygons;
};
} // namespace CesiumVectorData