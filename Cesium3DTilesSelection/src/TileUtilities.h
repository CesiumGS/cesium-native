#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
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
        cartographicPolygons,
    const CesiumGeospatial::Ellipsoid& ellipsoid
        CESIUM_DEFAULT_ELLIPSOID) noexcept;

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
        cartographicPolygons,
    const CesiumGeospatial::Ellipsoid& ellipsoid
        CESIUM_DEFAULT_ELLIPSOID) noexcept;
} // namespace CesiumImpl
} // namespace Cesium3DTilesSelection
