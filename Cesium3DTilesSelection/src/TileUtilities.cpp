#include "TileUtilities.h"

#include <variant>

#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h"
#include "CesiumGeospatial/GlobeRectangle.h"

namespace Cesium3DTilesSelection {
namespace Impl {

const CesiumGeospatial::GlobeRectangle* obtainGlobeRectangle(
    const Cesium3DTilesSelection::BoundingVolume* pBoundingVolume) noexcept {
  const CesiumGeospatial::BoundingRegion* pRegion =
      std::get_if<CesiumGeospatial::BoundingRegion>(pBoundingVolume);
  if (pRegion) {
    return &pRegion->getRectangle();
  }
  const CesiumGeospatial::BoundingRegionWithLooseFittingHeights* pLooseRegion =
      std::get_if<CesiumGeospatial::BoundingRegionWithLooseFittingHeights>(
          pBoundingVolume);
  if (pLooseRegion) {
    return &pLooseRegion->getBoundingRegion().getRectangle();
  }
  return nullptr;
}

bool withinPolygons(
    const BoundingVolume& boundingVolume,
    const std::vector<CartographicSelection>& cartographicSelections) noexcept {

  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Cesium3DTilesSelection::Impl::obtainGlobeRectangle(&boundingVolume);
  if (!pRectangle) {
    return false;
  }

  return withinPolygons(*pRectangle, cartographicSelections);
}

bool withinPolygons(
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const std::vector<CartographicSelection>& cartographicSelections) noexcept {

  glm::dvec2 rectangleCorners[] = {
      glm::dvec2(rectangle.getWest(), rectangle.getSouth()),
      glm::dvec2(rectangle.getWest(), rectangle.getNorth()),
      glm::dvec2(rectangle.getEast(), rectangle.getNorth()),
      glm::dvec2(rectangle.getEast(), rectangle.getSouth())};

  glm::dvec2 rectangleEdges[] = {
      rectangleCorners[1] - rectangleCorners[0],
      rectangleCorners[2] - rectangleCorners[1],
      rectangleCorners[3] - rectangleCorners[2],
      rectangleCorners[0] - rectangleCorners[3]};

  // Iterate through all polygons.
  for (size_t i = 0; i < cartographicSelections.size(); ++i) {
    const CartographicSelection& selection = cartographicSelections[i];

    const std::optional<CesiumGeospatial::GlobeRectangle>&
        polygonBoundingRectangle = selection.getBoundingRectangle();
    if (!polygonBoundingRectangle ||
        !rectangle.computeIntersection(*polygonBoundingRectangle)) {
      continue;
    }

    const std::vector<glm::dvec2>& vertices = selection.getVertices();
    const std::vector<uint32_t>& indices = selection.getIndices();

    // First check if an arbitrary point on the bounding globe rectangle is
    // inside the polygon.
    bool inside = false;
    for (size_t j = 2; j < indices.size(); j += 3) {
      const glm::dvec2& a = vertices[indices[j - 2]];
      const glm::dvec2& b = vertices[indices[j - 1]];
      const glm::dvec2& c = vertices[indices[j]];

      glm::dvec2 ab = b - a;
      glm::dvec2 ab_perp(-ab.y, ab.x);
      glm::dvec2 bc = c - b;
      glm::dvec2 bc_perp(-bc.y, bc.x);
      glm::dvec2 ca = a - c;
      glm::dvec2 ca_perp(-ca.y, ca.x);

      glm::dvec2 av = rectangleCorners[0] - a;
      glm::dvec2 cv = rectangleCorners[0] - c;

      double v_proj_ab_perp = glm::dot(av, ab_perp);
      double v_proj_bc_perp = glm::dot(cv, bc_perp);
      double v_proj_ca_perp = glm::dot(cv, ca_perp);

      // This will determine in or out, irrespective of winding.
      if ((v_proj_ab_perp >= 0.0 && v_proj_ca_perp >= 0.0 &&
           v_proj_bc_perp >= 0.0) ||
          (v_proj_ab_perp <= 0.0 && v_proj_ca_perp <= 0.0 &&
           v_proj_bc_perp <= 0.0)) {
        inside = true;
        break;
      }
    }

    // If the arbitrary point was outside, then this polygon does not entirely
    // cull the tile.
    if (!inside) {
      continue;
    }

    // Check if the polygon perimeter intersects the bounding globe rectangle
    // edges.
    bool intersectionFound = false;
    for (size_t j = 0; j < vertices.size(); ++j) {
      const glm::dvec2& a = vertices[j];
      const glm::dvec2& b = vertices[(j + 1) % vertices.size()];

      glm::dvec2 ba = a - b;

      // Check each rectangle edge.
      for (size_t k = 0; k < 4; ++k) {
        const glm::dvec2& cd = rectangleEdges[k];
        glm::dmat2 lineSegmentMatrix(cd, ba);
        glm::dvec2 ca = a - rectangleCorners[k];

        // s and t are calculated such that:
        // line_intersection = a + t * ab = c + s * cd
        glm::dvec2 st = glm::inverse(lineSegmentMatrix) * ca;

        // check that the intersection is within the line segments
        if (st.x <= 1.0 && st.x >= 0.0 && st.y <= 1.0 && st.y >= 0.0) {
          intersectionFound = true;
          break;
        }
      }

      if (intersectionFound) {
        break;
      }
    }

    // There is no intersection with the perimeter and at least one point is
    // inside the polygon so the tile is completely inside this polygon.
    if (!intersectionFound) {
      return true;
    }
  }

  return false;
}
} // namespace Impl

} // namespace Cesium3DTilesSelection
