#pragma once

#include <variant>
#include "Cesium3DTiles/Library.h"
#include "CesiumGeometry/OrientedBoundingBox.h"
#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeometry/BoundingSphere.h"
#include "CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h"

namespace Cesium3DTiles {

    /** 
     * @brief A bounding volume.
     *
     * This is a `std::variant` for different types of bounding volumes.
     *
     * @see CesiumGeometry::BoundingSphere
     * @see CesiumGeometry::OrientedBoundingBox
     * @see CesiumGeospatial::BoundingRegion
     * @see CesiumGeospatial::BoundingRegionWithLooseFittingHeights
     */
    typedef std::variant<
        CesiumGeometry::BoundingSphere,
        CesiumGeometry::OrientedBoundingBox,
        CesiumGeospatial::BoundingRegion,
        CesiumGeospatial::BoundingRegionWithLooseFittingHeights
    > BoundingVolume;

    /**
     * @brief Transform the given {@link BoundingVolume} with the given matrix.
     * 
     * If the given bounding volume is a {@link CesiumGeometry::BoundingSphere}
     * or {@link CesiumGeometry::OrientedBoundingBox}, then it will be transformed
     * with the given matrix. Bounding regions will not be transformed.
     * 
     * @param transform The transform matrix.
     * @param boundingVolume The bounding volume.
     */
    CESIUM3DTILES_API BoundingVolume transformBoundingVolume(const glm::dmat4x4& transform, const BoundingVolume& boundingVolume);

    /**
     * @brief Returns the center of the given  {@link BoundingVolume}
     * 
     * @param boundingVolume The bounding volume
     * @return The center point
     */
    CESIUM3DTILES_API glm::dvec3 getBoundingVolumeCenter(const BoundingVolume& boundingVolume);
}
