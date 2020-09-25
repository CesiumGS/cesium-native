#pragma once

#include "CesiumGeospatial/Library.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeospatial {

    class Cartographic;

    /**
     * A simple map projection where longitude and latitude are linearly mapped to X and Y by multiplying
     * them (in radians) by the {@see Ellipsoid::getMaximumRadius}. This projection is commonly known as geographic,
     * equirectangular, equidistant cylindrical, or plate carrée. It is also known as EPSG:4326.
     * 
     * @see WebMercatorProjection
     */
    class CESIUMGEOSPATIAL_API GeographicProjection {
    public:
        /**
         * @brief The maximum bounding rectangle of the geographic projection,
         * ranging from -PI to PI radians longitude and
         * from -PI/2 to +PI/2 radians latitude.
         */
        static const GlobeRectangle MAXIMUM_GLOBE_RECTANGLE;

        static CesiumGeometry::Rectangle computeMaximumProjectedRectangle(const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

        /**
         * @brief Constructs a new instance.
         * 
         * @param ellipsoid The ellipsoid.
         */
        GeographicProjection(const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

        /**
         * @brief Gets the ellipsoid.
         */
        const Ellipsoid& getEllipsoid() const { return this->_ellipsoid; }

        /**
         * Converts geodetic ellipsoid coordinates, in radians, to the equivalent geographic
         * X, Y, Z coordinates expressed in meters.  The height is copied unmodified to the `z` coordinate.
         *
         * @param cartographic The geodetic coordinates in radians.
         * @returns The equivalent geographic X, Y, Z coordinates, in meters.
         */
        glm::dvec3 project(const Cartographic& cartographic) const;

        /**
         * Projects a globe rectangle to geographic coordinates by projecting the southwest and northeast corners.
         * 
         * @param rectangle The globe rectangle to project.
         * @return The projected rectangle.
         */
        CesiumGeometry::Rectangle project(const CesiumGeospatial::GlobeRectangle& rectangle) const;

        /**
         * Converts geographicr X and Y coordinates, expressed in meters, to a {@link Cartographic}
         * containing geodetic ellipsoid coordinates. The height is set to 0.0.
         *
         * @param projectedCoordinates The geographic projected coordinates to unproject.
         * @returns The equivalent cartographic coordinates.
         */
        Cartographic unproject(const glm::dvec2& projectedCoordinates) const;

        /**
         * Converts geographic X, Y coordinates, expressed in meters, to a {@link Cartographic}
         * containing geodetic ellipsoid coordinates. The Z coordinate is copied unmodified to the
         * height.
         *
         * @param projectedCoordinates The geographic projected coordinates to unproject, with height (z) in meters.
         * @returns The equivalent cartographic coordinates.
         */
        Cartographic unproject(const glm::dvec3& projectedCoordinates) const;

        /**
         * Unprojects a geographic rectangle to the globe by unprojecting the southwest and northeast corners.
         * 
         * @param rectangle The rectangle to unproject.
         * @returns The unprojected rectangle.
         */
        CesiumGeospatial::GlobeRectangle unproject(const CesiumGeometry::Rectangle& rectangle) const;

        bool operator==(const GeographicProjection& rhs) const {
            return this->_ellipsoid == rhs._ellipsoid;
        };

        bool operator!=(const GeographicProjection& rhs) const {
            return this->_ellipsoid != rhs._ellipsoid;
        };

    private:
        Ellipsoid _ellipsoid;
        double _semimajorAxis;
        double _oneOverSemimajorAxis;
    };

}
