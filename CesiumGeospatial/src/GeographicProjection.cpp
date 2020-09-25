#include "CesiumGeospatial/GeographicProjection.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumUtility/Math.h"

namespace CesiumGeospatial {

    /*static*/ const GlobeRectangle GeographicProjection::MAXIMUM_GLOBE_RECTANGLE = GlobeRectangle(
        -CesiumUtility::Math::ONE_PI,
        -CesiumUtility::Math::PI_OVER_TWO,
        CesiumUtility::Math::ONE_PI,
        CesiumUtility::Math::PI_OVER_TWO
    );

    /*static*/ CesiumGeometry::Rectangle GeographicProjection::computeMaximumProjectedRectangle(const Ellipsoid& ellipsoid) {
        double longitudeValue = ellipsoid.getMaximumRadius() * CesiumUtility::Math::ONE_PI;
        double latitudeValue = ellipsoid.getMaximumRadius() * CesiumUtility::Math::PI_OVER_TWO;
        return CesiumGeometry::Rectangle(-longitudeValue, -latitudeValue, longitudeValue, latitudeValue);
    }

    GeographicProjection::GeographicProjection(const Ellipsoid& ellipsoid) :
        _ellipsoid(ellipsoid),
        _semimajorAxis(ellipsoid.getMaximumRadius()),
        _oneOverSemimajorAxis(1.0 / ellipsoid.getMaximumRadius())
    {
    }

    glm::dvec3 GeographicProjection::project(const Cartographic& cartographic) const {
        double semimajorAxis = this->_semimajorAxis;
        return glm::dvec3(
            cartographic.longitude * semimajorAxis,
            cartographic.latitude * semimajorAxis,
            cartographic.height
        );
    }

    CesiumGeometry::Rectangle GeographicProjection::project(const CesiumGeospatial::GlobeRectangle& rectangle) const {
        glm::dvec3 sw = this->project(rectangle.getSouthwest());
        glm::dvec3 ne = this->project(rectangle.getNortheast());
        return CesiumGeometry::Rectangle(sw.x, sw.y, ne.x, ne.y);
    }

    Cartographic GeographicProjection::unproject(const glm::dvec2& projectedCoordinates) const {
        double oneOverEarthSemimajorAxis = this->_oneOverSemimajorAxis;

        return Cartographic(
            projectedCoordinates.x * oneOverEarthSemimajorAxis,
            projectedCoordinates.y * oneOverEarthSemimajorAxis,
            0.0
        );
    }

    Cartographic GeographicProjection::unproject(const glm::dvec3& projectedCoordinates) const {
        Cartographic result = this->unproject(glm::dvec2(projectedCoordinates));
        result.height = projectedCoordinates.z;
        return result;
    }

    CesiumGeospatial::GlobeRectangle GeographicProjection::unproject(const CesiumGeometry::Rectangle& rectangle) const {
        Cartographic sw = this->unproject(rectangle.getLowerLeft());
        Cartographic ne = this->unproject(rectangle.getUpperRight());
        return GlobeRectangle(sw.longitude, sw.latitude, ne.longitude, ne.latitude);
    }

}
