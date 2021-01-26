#pragma once

#include "CesiumGeometry/Plane.h"

namespace Cesium3DTiles {

    /**
     * @brief A view frustum, defined by four planes.
     * 
     * The planes describe the view frustum of a camera.
     * The normals of these planes will point inwards.
     */
    struct Frustum final {
        const CesiumGeometry::Plane _leftPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
        const CesiumGeometry::Plane _rightPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
        const CesiumGeometry::Plane _topPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
        const CesiumGeometry::Plane _bottomPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
    };

    Frustum createFrustum(
        const glm::dvec3& position,
        const glm::dvec3& direction,
        const glm::dvec3& up,
        const double fovx,
        const double fovy);
}
