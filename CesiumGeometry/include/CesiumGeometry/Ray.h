// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include <glm/vec3.hpp>
#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {

    /**
     * @brief A ray that extends infinitely from the provided origin in the provided direction.
     */
    class CESIUMGEOMETRY_API Ray final {
    public:
        /**
         * @brief Construct a new ray.
         * 
         * @param origin The origin of the ray.
         * @param direction The direction of the ray (normalized).
         * 
         * @exception std::exception `direction` must be normalized.
         */
        Ray(const glm::dvec3& origin, const glm::dvec3& direction);

        /**
         * @brief Gets the origin of the ray.
         */
        const glm::dvec3& getOrigin() const noexcept { return this->_origin; }

        /**
         * @brief Gets the direction of the ray.
         */
        const glm::dvec3& getDirection() const noexcept { return this->_direction; }

        /**
         * @brief Constructs a new ray with its direction opposite this one.
         */
        Ray operator-() const noexcept;

    private:
        glm::dvec3 _origin;
        glm::dvec3 _direction;
    };
}
