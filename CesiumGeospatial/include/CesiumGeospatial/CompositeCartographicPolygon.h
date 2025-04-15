#pragma once

#include <CesiumGeospatial/CartographicPolygon.h>

namespace CesiumGeospatial {
/**
 * @brief A composite cartographic polygon made up of multiple
 * `CartographicPolygon` objects, each representing a different linear ring
 * within one larger polygon.
 *
 * Read more about linear rings:
 * https://esri.github.io/geometry-api-java/doc/Polygon.html
 */
class CompositeCartographicPolygon {
public:
  /**
   * @brief Creates a new `CompositeCartographicPolygon` from the provided list
   * of polygons.
   *
   * Each polygon serves as a linear ring within the total composite polygon.
   * The first provided polygon will be treated as the outer ring, and any
   * following polygons will be treated as inner rings.
   *
   * In other words, the first provided `CartographicPolygon` will delineate the
   * outer bounds of the composite polygon surface, and any further
   * `CartographicPolygon` objects will delineate holes within that surface.
   *
   * While conceptually it makes sense to have multiple outer rings within the
   * same composite polygon, in practice this is not supported. If this behavior
   * is needed, multiple composite polygons can be used instead.
   *
   * @param polygons The set of polygons defining the outer ring and any inner
   * rings of this composite polygon.
   */
  CompositeCartographicPolygon(std::vector<CartographicPolygon>&& polygons);

  /**
   * @brief Returns whether the given longitude-latitude point lies within this
   * composite polygon.
   */
  bool contains(const CesiumGeospatial::Cartographic& point) const;

  /**
   * @brief Triangulates this composite polygon, returning the indices of the
   * generated triangles.
   */
  std::vector<uint32_t> triangulate() const;

  /**
   * @brief Returns the linear rings that make up this composite polygon.
   */
  const std::vector<CartographicPolygon>& getLinearRings() const;

  /**
   * @brief Checks if two `CompositeCartographicPolygon` objects are equal.
   */
  bool operator==(const CompositeCartographicPolygon& rhs) const;

private:
  std::vector<CartographicPolygon> _polygons;
};
} // namespace CesiumGeospatial