#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "CesiumGeospatial/GlobeRectangle.h"

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
} // namespace Impl

} // namespace Cesium3DTiles
