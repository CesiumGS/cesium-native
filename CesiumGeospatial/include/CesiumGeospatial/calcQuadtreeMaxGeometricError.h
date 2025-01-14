#pragma once

#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Ellipsoid.h>

namespace CesiumGeospatial {
/**
 * @brief Computes the maximum geometric error per radian of a quadtree with
 * certain assumptions.
 *
 * The geometric error for a tile can be computed by multiplying the value
 * returned by this function by the width of the tile in radians.
 *
 * This function computes a suitable geometric error for a 65x65 terrain
 * heightmap where the vertical error is 25% of the horizontal spacing between
 * height samples at the equator.
 *
 * @param ellipsoid The ellipsoid.
 * @return The max geometric error.
 */
double calcQuadtreeMaxGeometricError(
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept;
} // namespace CesiumGeospatial
