#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"

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
} // namespace CesiumImpl
} // namespace Cesium3DTilesSelection
