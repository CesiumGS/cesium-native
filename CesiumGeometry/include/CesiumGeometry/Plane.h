#pragma once

#include <glm/vec3.hpp>
#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {

    /**
     * @brief A plane in Hessian Normal Format.
     */
    class CESIUMGEOMETRY_API Plane {
    public:
        /**
         * @brief Constructs a new plane from a normal and a distance from the origin.
         * 
         * The plane is defined by:
         * ```
         * ax + by + cz + d = 0
         * ```
         * where (a, b, c) is the plane's `normal`, d is the signed
         * `distance` to the plane, and (x, y, z) is any point on
         * the plane.
         * 
         * @param normal The plane's normal (normalized).
         * @param distance The shortest distance from the origin to the plane. The sign of
         * `distance` determines which side of the plane the origin
         * is on. If `distance` is positive, the origin is in the half-space
         * in the direction of the normal; if negative, the origin is in the half-space
         * opposite to the normal; if zero, the plane passes through the origin.
         * 
         * @exception std::exception `normal` must be normalized.
         *
         * Example:
         * @snippet TestPlane.cpp constructor-normal-distance
         */
        Plane(const glm::dvec3& normal, double distance);

        /**
         * @brief Construct a new plane from a point in the plane and the plane's normal.
         * 
         * @param point The point on the plane.
         * @param normal The plane's normal (normalized).
         *
         * @exception std::exception `normal` must be normalized.
         *
         * Example:
         * @snippet TestPlane.cpp constructor-point-normal
         */
        Plane(const glm::dvec3& point, const glm::dvec3& normal);

        /**
         * @brief Gets the plane's normal.
         */
        const glm::dvec3& getNormal() const { return this->_normal; }

        /**
         * @brief Gets the signed shortest distance from the origin to the plane.
         * The sign of `distance` determines which side of the plane the origin
         * is on.  If `distance` is positive, the origin is in the half-space
         * in the direction of the normal; if negative, the origin is in the half-space
         * opposite to the normal; if zero, the plane passes through the origin.
         */
        double getDistance() const { return this->_distance; }

        /**
         * @brief Computes the signed shortest distance of a point to this plane.
         * The sign of the distance determines which side of the plane the point
         * is on.  If the distance is positive, the point is in the half-space
         * in the direction of the normal; if negative, the point is in the half-space
         * opposite to the normal; if zero, the plane passes through the point.
         *
         * @param plane The plane.
         * @param point The point.
         * @returns The signed shortest distance of the point to the plane.
         */
        double getPointDistance(const glm::dvec3& point) const;

        /**
         * @brief Projects a point onto this plane.
         * @param point The point to project onto the plane.
         * @returns The projected point.
         */
        glm::dvec3 projectPointOntoPlane(const glm::dvec3& point) const;

    private:
        glm::dvec3 _normal;
        double _distance;
    };
}
