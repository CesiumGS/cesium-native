#pragma once

#include "CesiumGeospatial/Cartographic.h"

#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Library.h>

#include <glm/vec2.hpp>

#include <optional>
#include <string>
#include <vector>

namespace CesiumGeospatial {

/**
 * @brief A 2D polygon expressed as a list of longitude/latitude coordinates in
 * radians.
 *
 * The {@link Ellipsoid} associated with the coordinates is not specified
 * directly by this instance, but it is assumed that the longitude values range
 * from -PI to PI radians and the latitude values range from -PI/2 to PI/2
 * radians. Longitude values outside this range are wrapped to be inside the
 * range. Latitude values are clamped to the range.
 */
class CESIUMGEOSPATIAL_API CartographicPolygon final {
public:
  /**
   * @brief Constructs a 2D polygon that can be rasterized onto {@link Cesium3DTilesSelection::Tileset}
   * objects.
   *
   * @param polygon An array of longitude-latitude points in radians defining
   * the perimeter of the 2D polygon.
   */
  CartographicPolygon(const std::vector<glm::dvec2>& polygon);

  /**
   * @brief Constructs a 2D polygon that can be rasterized onto {@link Cesium3DTilesSelection::Tileset}
   * objects.
   *
   * @param polygon An array of cartographic points defining the perimeter of
   * the 2D polygon. The 'height' component of each coordinate will be ignored.
   */
  CartographicPolygon(const std::vector<Cartographic>& polygon);

  /**
   * @brief Constructs a 2D polygon with the linear ring defining its boundary
   * and the set of linear rings defining its holes.
   *
   * @param vertices The vertices defining the outer boundary of the polygon.
   * @param holes The polygons defining holes within this polygon. Any holes in
   * those polygons are then inner polygons, and so on.
   */
  CartographicPolygon(
      const std::vector<Cartographic>& vertices,
      std::vector<CartographicPolygon>&& holes);

  /**
   * @brief Returns the longitude-latitude vertices that define the
   * perimeter of the selected polygon.
   *
   * @return The perimeter vertices in longitude-latitude radians.
   */
  constexpr const std::vector<glm::dvec2>& getVertices() const {
    return this->_vertices;
  }

  /**
   * @brief Returns the triangulated indices representing a triangle
   * decomposition of the polygon. The indices are in reference to the
   * polygon's vertices, including holes.
   *
   * For example, for a polygon made up of `( v0, v1, v2 )` with holes made up
   * of `( v4, v5, v6 )` and `( v7, v8, v9 )`, index 1 would correspond to v1,
   * index 5 to v5, and so on.
   *
   * @remarks Due to limitations of the triangulation algorithm, any inner
   * polygons contained in the holes of this polygon will be ignored.
   *
   * @return The indices for the polygon's triangle decomposition.
   */
  constexpr const std::vector<uint32_t>& getIndices() const {
    return this->_indices;
  }

  /**
   * @brief Returns a {@link GlobeRectangle} that represents the bounding
   * rectangle of the polygon.
   *
   * @return The polygon's global bounding rectangle.
   */
  constexpr const CesiumGeospatial::GlobeRectangle&
  getBoundingRectangle() const {
    return this->_boundingRectangle;
  }

  /**
   * @brief Returns the \ref CartographicPolygon instances that define holes in
   * this polygon.
   */
  constexpr const std::vector<CartographicPolygon>& getHoles() const {
    return this->_holes;
  }

  /**
   * @brief Returns whether the given longitude-latitude position lies within
   * this polygon.
   */
  bool contains(const CesiumGeospatial::Cartographic& point) const;

  /**
   * @brief Computes the winding order of this polygon. If true, the polygon's
   * vertices are in clockwise order. If false, the polygon's vertices are in
   * counter-clockwise order.
   */
  bool isClockwiseWindingOrder() const;

  /**
   * @brief Determines whether a globe rectangle is completely inside any of the
   * polygons in a list.
   *
   * @warning Does not take into account any holes in this polygon.
   *
   * @param rectangle The {@link CesiumGeospatial::GlobeRectangle} of the tile.
   * @param cartographicPolygons The list of polygons to check.
   * @return True if the rectangle is completely inside a polygon; otherwise,
   * false.
   */
  static bool rectangleIsWithinPolygons(
      const CesiumGeospatial::GlobeRectangle& rectangle,
      const std::vector<CesiumGeospatial::CartographicPolygon>&
          cartographicPolygons) noexcept;

  /**
   * @brief Determines whether a globe rectangle is completely outside all the
   * polygons in a list.
   *
   * @warning Does not take into account any holes in this polygon.
   *
   * @param rectangle The {@link CesiumGeospatial::GlobeRectangle} of the tile.
   * @param cartographicPolygons The list of polygons to check.
   * @return True if the rectangle is completely outside all the polygons;
   * otherwise, false.
   */
  static bool rectangleIsOutsidePolygons(
      const CesiumGeospatial::GlobeRectangle& rectangle,
      const std::vector<CesiumGeospatial::CartographicPolygon>&
          cartographicPolygons) noexcept;

  /**
   * @brief Checks if two `CartographicPolygon` objects are equal.
   */
  bool operator==(const CartographicPolygon& rhs) const;

private:
  std::vector<glm::dvec2> _vertices;
  std::vector<uint32_t> _indices;
  std::vector<CartographicPolygon> _holes;
  CesiumGeospatial::GlobeRectangle _boundingRectangle;
};

} // namespace CesiumGeospatial
