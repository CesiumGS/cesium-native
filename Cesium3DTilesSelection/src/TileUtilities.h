#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"

#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>

#include <vector>

namespace Cesium3DTilesSelection {
namespace CesiumImpl {

/**
 * @brief Returns whether the point is completely inside the triangle.
 *
 * @param point The point to check.
 * @param triangleVertA The first vertex of the triangle.
 * @param triangleVertB The second vertex of the triangle.
 * @param triangleVertC The third vertex of the triangle.
 * @return Whether the point is within the triangle.
 */
bool withinTriangle(
    const glm::dvec2& point,
    const glm::dvec2& triangleVertA,
    const glm::dvec2& triangleVertB,
    const glm::dvec2& triangleVertC) noexcept;

/**
 * @brief Returns whether the tile is completely inside a polygon.
 *
 * @param boundingVolume The {@link Cesium3DTilesSelection::BoundingVolume} of the tile.
 * @param cartographicPolygons The list of polygons to check.
 * @return Whether the tile is completely inside a polygon.
 */
bool withinPolygons(
    const BoundingVolume& boundingVolume,
    const std::vector<CesiumGeospatial::CartographicPolygon>&
        cartographicPolygons) noexcept;

/**
 * @brief Returns whether the tile is completely inside a polygon.
 *
 * @param rectangle The {@link CesiumGeospatial::GlobeRectangle} of the tile.
 * @param cartographicPolygons The list of polygons to check.
 * @return Whether the tile is completely inside a polygon.
 */
bool withinPolygons(
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const std::vector<CesiumGeospatial::CartographicPolygon>&
        cartographicPolygons) noexcept;

/**
 * @brief Returns whether the tile is completely outside all the polygons.
 *
 * @param boundingVolume The {@link Cesium3DTilesSelection::BoundingVolume} of the tile.
 * @param cartographicPolygons The list of polygons to check.
 * @return Whether the tile is completely outside all the polygons.
 */
bool outsidePolygons(
    const BoundingVolume& boundingVolume,
    const std::vector<CesiumGeospatial::CartographicPolygon>&
        cartographicPolygons) noexcept;

/**
 * @brief Returns whether the tile is completely outside all the polygons.
 *
 * @param rectangle The {@link CesiumGeospatial::GlobeRectangle} of the tile.
 * @param cartographicPolygons The list of polygons to check.
 * @return Whether the tile is completely outside all the polygons.
 */
bool outsidePolygons(
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const std::vector<CesiumGeospatial::CartographicPolygon>&) noexcept;
} // namespace CesiumImpl
} // namespace Cesium3DTilesSelection
