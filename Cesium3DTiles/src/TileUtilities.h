#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/CartographicSelection.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include <vector>

namespace Cesium3DTiles {
namespace Impl {

// TODO: This could be considered to become a function of Tile (even though
// it currently does not need a tile, but only a bounding volume...)

/**
 * @brief Obtains the {@link CesiumGeospatial::GlobeRectangle} from the given
 * {@link Cesium3DTiles::BoundingVolume}.
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
 * Cesium3DTiles::BoundingVolume}.
 * @return The {@link CesiumGeospatial::GlobeRectangle}, or `nullptr`
 */
const CesiumGeospatial::GlobeRectangle* obtainGlobeRectangle(
    const Cesium3DTiles::BoundingVolume* pBoundingVolume) noexcept;

/**
 * @brief Returns whether the tile is completely inside a polygon.
 *
 * @param boundingVolume The {@link Cesium3DTiles::BoundingVolume} of the tile.
 * @param cartographicSelections The list of polygon selections to check.
 * @return Whether the tile is completely inside a polygon.
 */
bool withinPolygons(
    const BoundingVolume& boundingVolume,
    const std::vector<CartographicSelection>& cartographicSelections) noexcept;

/**
 * @brief Returns whether the tile is completely inside a polygon.
 *
 * @param rectangle The {@link CesiumGeospatial::GlobeRectangle} of the tile.
 * @param cartographicSelections The list of polygon selections to check.
 * @return Whether the tile is completely inside a polygon.
 */
bool withinPolygons(
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const std::vector<CartographicSelection>& cartographicSelections) noexcept;
} // namespace Impl
} // namespace Cesium3DTiles
