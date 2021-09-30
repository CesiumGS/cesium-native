#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"

#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>

#include <vector>

namespace Cesium3DTilesSelection {
namespace Impl {

// TODO: This could be considered to become a function of Tile (even though
// it currently does not need a tile, but only a bounding volume...)

/**
 * @brief Obtains the {@link CesiumGeospatial::GlobeRectangle} from the given
 * {@link Cesium3DTilesSelection::BoundingVolume}.
 *
 * If the given bounding volume is a {@link CesiumGeospatial::BoundingRegion},
 * then its rectangle will be returned.
 *
 * If the given bounding volume is a {@link
 * CesiumGeospatial::BoundingRegionWithLooseFittingHeights}, then the rectangle
 * of its bounding region  will be returned.
 *
 * Otherwise, `nullptr` will be returned.
 *
 * @param pBoundingVolume A pointer to the {@link
 * Cesium3DTilesSelection::BoundingVolume}.
 * @return The {@link CesiumGeospatial::GlobeRectangle}, or `nullptr`
 */
const CesiumGeospatial::GlobeRectangle* obtainGlobeRectangle(
    const Cesium3DTilesSelection::BoundingVolume* pBoundingVolume) noexcept;

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
} // namespace Impl
} // namespace Cesium3DTilesSelection
