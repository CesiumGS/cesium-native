#pragma once

#include "CesiumGeospatial/Library.h"
#include <glm/vec3.hpp>
#include <optional>
#include "CesiumGeospatial/Cartographic.h"

namespace CesiumGeospatial {

    /**
     * @brief A quadratic surface defined in Cartesian coordinates.
     * 
     * The surface is defined by the equation `(x / a)^2 + (y / b)^2 + (z / c)^2 = 1`. 
     * This is primarily used by Cesium to represent the shape of planetary bodies. 
     * Rather than constructing this object directly, one of the provided constants 
     * is normally used.
     */
    class CESIUMGEOSPATIAL_API  Ellipsoid {
    public:

        /**
         * @brief An Ellipsoid instance initialized to the WGS84 standard.
         *
         * The ellipsoid is initialized to the  World Geodetic System (WGS84) standard, as defined
         * in https://earth-info.nga.mil/GandG/publications/tr8350.2/wgs84fin.pdf.
         */
        static const Ellipsoid WGS84;

        /**
         * @brief Creates a new instance.
         * 
         * @param x The radius in x-direction.
         * @param y The radius in y-direction.
         * @param z The radius in z-direction.
         */
        Ellipsoid(double x, double y, double z);

        /**
         * @brief Creates a new instance.
         *
         * @param radii The radii in x-, y-, and z-direction.
         */
        Ellipsoid(const glm::dvec3& radii);

        /**
         * @brief Returns the radii in x-, y-, and z-direction.
         */
        const glm::dvec3& getRadii() const { return this->_radii; }

        /**
         * @brief Computes the normal of the plane tangent to the surface of the ellipsoid at the provided position.
         * 
         * @param position The cartesian position for which to to determine the surface normal.
         * @return The normal
         */
        glm::dvec3 geodeticSurfaceNormal(const glm::dvec3& position) const;

        /**
         * @brief Computes the normal of the plane tangent to the surface of the ellipsoid at the provided position.
         *
         * @param cartographic The {@link Cartographic} position for which to to determine the surface normal.
         * @return The normal
         */
        glm::dvec3 geodeticSurfaceNormal(const Cartographic& cartographic) const;

        /**
         * @brief Converts the provided {@link Cartographic} to cartesian representation.
         *
         * @param cartographic The {@link Cartographic} position.
         * @return The cartesian representation
         */
        glm::dvec3 cartographicToCartesian(const Cartographic& cartographic) const;

        /**
         * @brief Converts the provided cartesian to a {@link Cartographic} representation. 
         * 
         * The result will be the empty optional if the given cartesian is at the center of this ellipsoid.
         * 
         * @param cartesian The cartesian position
         * @return The {@link Cartographic} representation, or the empty optional if
         * the cartesian is at the center of this ellipsoid.
         */
        std::optional<Cartographic> cartesianToCartographic(const glm::dvec3& cartesian) const;

        /**
         * @brief Scales the given cartesian position along the geodetic surface normal so that it is on the surface of this ellipsoid. 
         *
         * The result will be the empty optional if the position is at the center of this ellipsoid.
         *
         * @param cartesian The cartesian position
         * @return The scaled position, or the empty optional if
         * the cartesian is at the center of this ellipsoid.
         */
        std::optional<glm::dvec3> scaleToGeodeticSurface(const glm::dvec3& cartesian) const;

        /**
         * @brief The maximum radius in any dimension.
         *
         * @return The maximum radius.
         */
        double getMaximumRadius() const;

        /**
         * @brief The minimum radius in any dimension.
         *
         * @return The minimum radius.
         */
        double getMinimumRadius() const;

        /**
         * @brief Returns `true` if two elliposids are equal
         */
        bool operator==(const Ellipsoid& rhs) const {
            return this->_radii == rhs._radii;
        };

        /**
         * @brief Returns `true` if two elliposids are *not* equal
         */
        bool operator!=(const Ellipsoid& rhs) const {
            return this->_radii != rhs._radii;
        };

    private:
        glm::dvec3 _radii;
        glm::dvec3 _radiiSquared;
        glm::dvec3 _oneOverRadii;
        glm::dvec3 _oneOverRadiiSquared;
        double _centerToleranceSquared;
    };

}
