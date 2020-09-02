#include "CesiumGeospatial/WebMercatorProjection.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumUtility/Math.h"

namespace CesiumGeospatial {

    /*static*/ const double WebMercatorProjection::MAXIMUM_LATITUDE = WebMercatorProjection::mercatorAngleToGeodeticLatitude(CesiumUtility::Math::ONE_PI);

    /*static*/ const GlobeRectangle WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE = GlobeRectangle(-CesiumUtility::Math::ONE_PI, -MAXIMUM_LATITUDE, CesiumUtility::Math::ONE_PI, MAXIMUM_LATITUDE);

    /*static*/ CesiumGeometry::Rectangle WebMercatorProjection::computeMaximumProjectedRectangle(const Ellipsoid& ellipsoid) {
        double value = ellipsoid.getMaximumRadius() * CesiumUtility::Math::ONE_PI;
        return CesiumGeometry::Rectangle(-value, -value, value, value);
    }

    WebMercatorProjection::WebMercatorProjection(const Ellipsoid& ellipsoid) :
        _ellipsoid(ellipsoid),
        _semimajorAxis(ellipsoid.getMaximumRadius()),
        _oneOverSemimajorAxis(1.0 / ellipsoid.getMaximumRadius())
    {

    }

    glm::dvec3 WebMercatorProjection::project(const Cartographic& cartographic) const {
        double semimajorAxis = this->_semimajorAxis;
        return glm::dvec3(
            cartographic.longitude * semimajorAxis,
            WebMercatorProjection::geodeticLatitudeToMercatorAngle(cartographic.latitude) * semimajorAxis,
            cartographic.height
        );
    }

    Cartographic WebMercatorProjection::unproject(const glm::dvec2& projectedCoordinates) const {
        double oneOverEarthSemimajorAxis = this->_oneOverSemimajorAxis;

        return Cartographic(
            projectedCoordinates.x * oneOverEarthSemimajorAxis,
            WebMercatorProjection::mercatorAngleToGeodeticLatitude(projectedCoordinates.y * oneOverEarthSemimajorAxis),
            0.0
        );
    }

    Cartographic WebMercatorProjection::unproject(const glm::dvec3& projectedCoordinates) const {
        Cartographic result = this->unproject(glm::dvec2(projectedCoordinates));
        result.height = projectedCoordinates.z;
        return result;
    }

    /*static*/ double WebMercatorProjection::mercatorAngleToGeodeticLatitude(double mercatorAngle) {
        return CesiumUtility::Math::PI_OVER_TWO - 2.0 * atan(exp(-mercatorAngle));
    }

    /*static*/ double WebMercatorProjection::geodeticLatitudeToMercatorAngle(double latitude) {
        // Clamp the latitude coordinate to the valid Mercator bounds.
        latitude = CesiumUtility::Math::clamp(latitude, -WebMercatorProjection::MAXIMUM_LATITUDE, WebMercatorProjection::MAXIMUM_LATITUDE);

        double sinLatitude = sin(latitude);
        return 0.5 * log((1.0 + sinLatitude) / (1.0 - sinLatitude));
    }

}
