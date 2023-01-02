#pragma once

#include "Library.h"

#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>

#include <optional>
#include <variant>

namespace Cesium3DTilesSelection {

/**
 * @brief A bounding volume.
 *
 * This is a `std::variant` for different types of bounding volumes.
 *
 * @see CesiumGeometry::BoundingSphere
 * @see CesiumGeometry::OrientedBoundingBox
 * @see CesiumGeospatial::BoundingRegion
 * @see CesiumGeospatial::BoundingRegionWithLooseFittingHeights
 * @see CesiumGeospatial::S2CellBoundingVolume
 */
typedef std::variant<
    CesiumGeometry::BoundingSphere,
    CesiumGeometry::OrientedBoundingBox,
    CesiumGeospatial::BoundingRegion,
    CesiumGeospatial::BoundingRegionWithLooseFittingHeights,
    CesiumGeospatial::S2CellBoundingVolume>
    BoundingVolume;

/**
 * @brief Transform the given {@link BoundingVolume} with the given matrix.
 *
 * If the given bounding volume is a {@link CesiumGeometry::BoundingSphere}
 * or {@link CesiumGeometry::OrientedBoundingBox}, then it will be transformed
 * with the given matrix. Bounding regions will not be transformed.
 *
 * @param transform The transform matrix.
 * @param boundingVolume The bounding volume.
 * @return The transformed bounding volume.
 */
CESIUM3DTILESSELECTION_API BoundingVolume transformBoundingVolume(
    const glm::dmat4x4& transform,
    const BoundingVolume& boundingVolume);

/**
 * @brief Returns the center of the given {@link BoundingVolume}.
 *
 * @param boundingVolume The bounding volume.
 * @return The center point.
 */
CESIUM3DTILESSELECTION_API glm::dvec3
getBoundingVolumeCenter(const BoundingVolume& boundingVolume);

/**
 * @brief Estimates the bounding {@link CesiumGeospatial::GlobeRectangle} of the
 * given {@link BoundingVolume}.
 *
 * @param boundingVolume The bounding volume.
 * @return The bounding {@link CesiumGeospatial::GlobeRectangle}.
 */
CESIUM3DTILESSELECTION_API std::optional<CesiumGeospatial::GlobeRectangle>
estimateGlobeRectangle(const BoundingVolume& boundingVolume);

/**
 * @brief Returns the bounding region if the bounding volume is a
 * {@link BoundingRegion} or a {@link BoundingRegionWithLooseFittingHeights}.
 *
 * @param boundingVolume The bounding volume.
 * @return A pointer to the bounding region, or nullptr is the bounding volume
 * is not a bounding region.
 */
CESIUM3DTILESSELECTION_API const CesiumGeospatial::BoundingRegion*
getBoundingRegionFromBoundingVolume(const BoundingVolume& boundingVolume);

} // namespace Cesium3DTilesSelection
