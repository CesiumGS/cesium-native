#pragma once

#include "CesiumGeospatial/Library.h"
#include <glm/vec3.hpp>
#include <optional>
#include "CesiumGeospatial/Cartographic.h"

namespace CesiumGeospatial {

    class CESIUMGEOSPATIAL_API  Ellipsoid {
    public:

        /**
         * @brief An Ellipsoid instance initialized to the WGS84 standard.
         *
         * The ellipsoid is initialized to the  World Geodetic System (WGS84) standard, as defined
         * in https://earth-info.nga.mil/GandG/publications/tr8350.2/wgs84fin.pdf.
         */
        static const Ellipsoid WGS84;

        Ellipsoid(double x, double y, double z);
        Ellipsoid(const glm::dvec3& radii);

        const glm::dvec3& getRadii() const { return this->_radii; }

        glm::dvec3 geodeticSurfaceNormal(const glm::dvec3& position) const;
        glm::dvec3 geodeticSurfaceNormal(const Cartographic& cartographic) const;
        glm::dvec3 cartographicToCartesian(const Cartographic& cartographic) const;
        std::optional<Cartographic> cartesianToCartographic(const glm::dvec3& cartesian) const;
        std::optional<glm::dvec3> scaleToGeodeticSurface(const glm::dvec3& cartesian) const;

        /**
         * @brief The maximum radius in any dimension
         *
         * @return The maximum radius
         */
        double getMaximumRadius() const;

        /**
         * @brief The minimum radius in any dimension
         *
         * @return The minimum radius
         */
        double getMinimumRadius() const;

        bool operator==(const Ellipsoid& rhs) const {
            return this->_radii == rhs._radii;
        };

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
