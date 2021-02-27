// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeospatial/Library.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "CesiumGeospatial/Ellipsoid.h"

namespace CesiumGeospatial {

    /**
     * @brief Transforms positions to various reference frames.
     */
    class CESIUMGEOSPATIAL_API Transforms final {
    public:

        /**
         * @brief Computes a transformation from east-north-up axes to an ellipsoid-fixed reference frame.
         * 
         * Computes a 4x4 transformation matrix from a reference frame with an east-north-up axes 
         * centered at the provided origin to the provided ellipsoid's fixed reference frame. 
         * The local axes are defined as:
         * <ul>
         *  <li>The `x` axis points in the local east direction.</li>
         *  <li>The `y` axis points in the local north direction.</li>
         *  <li>The `z` axis points in the direction of the ellipsoid surface normal which passes through the position.</li>
         * </ul>
         * 
         * @param origin The center point of the local reference frame.
         * @param ellipsoid The {@link Ellipsoid} whose fixed frame is used in the transformation. Default value: {@link Ellipsoid::WGS84}.
         * @return The transformation matrix
         */
        static glm::dmat4x4 eastNorthUpToFixedFrame(const glm::dvec3& origin, const Ellipsoid& ellipsoid = Ellipsoid::WGS84) noexcept;
    };

}
