#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"

#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>

#include <vector>

namespace Cesium3DTilesSelection {
namespace CesiumImpl {
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
 * @brief Returns whether the ray intersects the bounding volume.
 */
bool rayIntersectsBoundingVolume(
    const BoundingVolume& boundingVolume,
    const CesiumGeometry::Ray& ray);
} // namespace CesiumImpl
} // namespace Cesium3DTilesSelection
