#pragma once

#include "CesiumGeometry/Plane.h"

namespace Cesium3DTiles {

    /**
     * @brief A culling volume, defined by four planes.
     * 
     * The planes describe the culling volume that may be created for 
     * the view frustum of a camera. The normals of these planes will 
     * point inwards.
     */
    struct CullingVolume final {
        const CesiumGeometry::Plane _leftPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
        const CesiumGeometry::Plane _rightPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
        const CesiumGeometry::Plane _topPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
        const CesiumGeometry::Plane _bottomPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
    };

    /**
     * @brief Creates a {@link CullingVolume} for a perspective frustum.
     * 
     * @param position The eye position
     * @param direction The viewing direction
     * @param up The up-vector of the frustum
     * @param fovxRad The horizontal Field-Of-View angle, in radians
     * @param fovyRad The vertical Field-Of-View angle, in radians
     * @return The {@ling CullingVolume}
     */
    CullingVolume createCullingVolume(
        const glm::dvec3& position,
        const glm::dvec3& direction,
        const glm::dvec3& up,
        const double fovxRad,
        const double fovyRad);
}
