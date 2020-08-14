#pragma once

#include <glm/vec3.hpp>
#include "CesiumGeometry/Library.h"
#include "CullingResult.h"

namespace CesiumGeometry {

    class Plane;

    /**
     * @brief A bounding sphere with a center and a radius.
     */
    class CESIUMGEOMETRY_API BoundingSphere {
    public:
        /**
         * @brief Construct a new instance.
         * 
         * @param center The center of the bounding sphere.
         * @param radius The radius of the bounding sphere.
         */
        BoundingSphere(const glm::dvec3& center, double radius);

        /**
         * @brief Gets the center of the bounding sphere.
         */
        const glm::dvec3& getCenter() const { return this-> _center; }

        /**
         * @brief Gets the radius of the bounding sphere.
         */
        double getRadius() const { return this->_radius; }
        
        /**
         * @brief Determines on which side of a plane this boundings sphere is located.
         * 
         * @param plane The plane to test against.
         * @return
         *  * {@link CullingResult::Inside} if the entire sphere is on the side of the plane the normal is pointing.
         *  * {@link CullingResult::Outside} if the entire sphere is on the opposite side.
         *  * {@link CullingResult::Intersecting} if the sphere intersects the plane.
         */
        CullingResult intersectPlane(const Plane& plane) const;

        /**
         * @brief Computes the distance squared from a position to the closest point on this bounding sphere.
         * 
         * @param position The position.
         * @return The distance squared from the position to the closest point on this bounding sphere.
         */
        double computeDistanceSquaredToPosition(const glm::dvec3& position) const;

    private:
        glm::dvec3 _center;
        double _radius;
    };

}
