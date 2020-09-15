#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumUtility/Math.h"

using namespace CesiumUtility;

namespace CesiumGeospatial {

    GlobeRectangle::GlobeRectangle(
        double west,
        double south,
        double east,
        double north
    ) :
        _west(west),
        _south(south),
        _east(east),
        _north(north)
    {
    }

    /*static*/ GlobeRectangle GlobeRectangle::fromDegrees(double westDegrees, double southDegrees, double eastDegrees, double northDegrees) {
        return GlobeRectangle(
            Math::degreesToRadians(westDegrees),
            Math::degreesToRadians(southDegrees),
            Math::degreesToRadians(eastDegrees),
            Math::degreesToRadians(northDegrees)
        );
    }

    CesiumGeometry::Rectangle GlobeRectangle::toSimpleRectangle() const {
        return CesiumGeometry::Rectangle(this->getWest(), this->getSouth(), this->getEast(), this->getNorth());
    }

    double GlobeRectangle::computeWidth() const {
        double east = this->_east;
        double west = this->_west;
        if (east < west) {
            east += Math::TWO_PI;
        }
        return east - west;
    }

    double GlobeRectangle::computeHeight() const {
        return this->_north - this->_south;
    }

    Cartographic GlobeRectangle::computeCenter() const {
        double east = this->_east;
        double west = this->_west;

        if (east < west) {
            east += Math::TWO_PI;
        }

        double longitude = Math::negativePiToPi((west + east) * 0.5);
        double latitude = (this->_south + this->_north) * 0.5;

        return Cartographic(longitude, latitude, 0.0);
    }

    bool GlobeRectangle::contains(const Cartographic& cartographic) const {
        double longitude = cartographic.longitude;
        double latitude = cartographic.latitude;

        double west = this->_west;
        double east = this->_east;

        if (east < west) {
            east += Math::TWO_PI;
            if (longitude < 0.0) {
                longitude += Math::TWO_PI;
            }
        }
        return (
            (longitude > west || Math::equalsEpsilon(longitude, west, Math::EPSILON14)) &&
            (longitude < east || Math::equalsEpsilon(longitude, east, Math::EPSILON14)) &&
            latitude >= this->_south &&
            latitude <= this->_north
        );
    }

    std::optional<GlobeRectangle> GlobeRectangle::intersect(const GlobeRectangle& other) const {
        double rectangleEast = this->_east;
        double rectangleWest = this->_west;

        double otherRectangleEast = other._east;
        double otherRectangleWest = other._west;

        if (rectangleEast < rectangleWest && otherRectangleEast > 0.0) {
            rectangleEast += CesiumUtility::Math::TWO_PI;
        } else if (otherRectangleEast < otherRectangleWest && rectangleEast > 0.0) {
            otherRectangleEast += CesiumUtility::Math::TWO_PI;
        }

        if (rectangleEast < rectangleWest && otherRectangleWest < 0.0) {
            otherRectangleWest += CesiumUtility::Math::TWO_PI;
        } else if (otherRectangleEast < otherRectangleWest && rectangleWest < 0.0) {
            rectangleWest += CesiumUtility::Math::TWO_PI;
        }

        double west = CesiumUtility::Math::negativePiToPi(
            std::max(rectangleWest, otherRectangleWest)
        );
        double east = CesiumUtility::Math::negativePiToPi(
            std::min(rectangleEast, otherRectangleEast)
        );

        if (
            (this->_west < this->_east ||
            other._west < other._east) &&
            east <= west
        ) {
            return std::nullopt;
        }

        double south = std::max(this->_south, other._south);
        double north = std::min(this->_north, other._north);

        if (south >= north) {
            return std::nullopt;
        }

        return GlobeRectangle(west, south, east, north);
    }

}
