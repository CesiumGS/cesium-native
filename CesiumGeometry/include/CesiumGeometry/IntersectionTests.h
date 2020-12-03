#pragma once

#include <optional>
#include <glm/vec3.hpp>
#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {
    class Ray;
    class Plane;

    /**
     * @brief Functions for computing the intersection between geometries such as rays, planes, triangles, and ellipsoids.
     */
    class CESIUMGEOMETRY_API IntersectionTests {
    public:
        /**
         * @brief Computes the intersection of a ray and a plane.
         * 
         * @param ray The ray.
         * @param plane The plane.
         * @return The point of intersection, or `std::nullopt` if there is no intersection.
         */
        static std::optional<glm::dvec3> rayPlane(const Ray& ray, const Plane& plane) noexcept;
    };

}
