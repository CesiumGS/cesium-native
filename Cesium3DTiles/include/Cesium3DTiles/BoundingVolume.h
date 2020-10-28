#pragma once

#include <variant>
#include "Cesium3DTiles/Library.h"
#include "CesiumGeometry/OrientedBoundingBox.h"
#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeometry/BoundingSphere.h"
#include "CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h"

namespace Cesium3DTiles {

    /** 
     * @brief A bounding volume
     *
     * This is a `std::variant` for different types of bounding volumes
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

    CESIUM3DTILES_API BoundingVolume transformBoundingVolume(const glm::dmat4x4& transform, const BoundingVolume& boundingVolume);
    CESIUM3DTILES_API glm::dvec3 getBoundingVolumeCenter(const BoundingVolume& boundingVolume);
}
