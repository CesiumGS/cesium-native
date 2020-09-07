#pragma once

#include "CesiumGeospatial/Library.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeometry/Rectangle.h"
#include <optional>

namespace CesiumGeospatial {

    /**
     * @brief A two-dimensional, recangular region on a globe, specified using longitude and latitude coordinates.
     * The region is rectangular in terms of longitude-latitude coordinates, but may be far from rectangular on the
     * actual globe surface.
     * 
     * @see CesiumGeometry::Rectangle
     */
    class CESIUMGEOSPATIAL_API GlobeRectangle {
    public:
        /**
         * @brief Constructs a new instance.
         * 
         * @param west The westernmost longitude, in radians, in the range [-Pi, Pi].
         * @param south The southernmost latitude, in radians, in the range [-Pi/2, Pi/2].
         * @param east The easternmost longitude, in radians, in the range [-Pi, Pi].
         * @param north The northernmost latitude, in radians, in the range [-Pi/2, Pi/2].
         */
        GlobeRectangle(
            double west,
            double south,
            double east,
            double north
        );

        /**
         * Creates a rectangle given the boundary longitude and latitude in degrees. The angles are converted to radians.
         *
         * @param west The westernmost longitude in degrees in the range [-180.0, 180.0].
         * @param south The southernmost latitude in degrees in the range [-90.0, 90.0].
         * @param east The easternmost longitude in degrees in the range [-180.0, 180.0].
         * @param north The northernmost latitude in degrees in the range [-90.0, 90.0].
         * @returns The rectangle.
         *
         * @snippet TestRectangle.cpp fromDegrees
         */
        static GlobeRectangle fromDegrees(double westDegrees, double southDegrees, double eastDegrees, double northDegrees);

        double getWest() const { return this->_west; }
        double getSouth() const { return this->_south; }
        double getEast() const { return this->_east; }
        double getNorth() const { return this->_north; }

        Cartographic getSouthwest() const { return Cartographic(this->_west, this->_south); }
        Cartographic getSoutheast() const { return Cartographic(this->_east, this->_south); }
        Cartographic getNorthwest() const { return Cartographic(this->_west, this->_north); }
        Cartographic getNortheast() const { return Cartographic(this->_east, this->_north); }

        CesiumGeometry::Rectangle toSimpleRectangle() const;

        double computeWidth() const;
        double computeHeight() const;
        Cartographic computeCenter() const;

        bool contains(const Cartographic& cartographic) const;

        /**
         * Computes the intersection of two rectangles.  This function assumes that the rectangle's coordinates are
         * latitude and longitude in radians and produces a correct intersection, taking into account the fact that
         * the same angle can be represented with multiple values as well as the wrapping of longitude at the
         * anti-meridian.  For a simple intersection that ignores these factors and can be used with projected
         * coordinates, see {@link Rectangle::intersect}.
         *
         * @param other The other rectangle to intersect with this one.
         * @returns The intersection rectangle, or `std::nullopt` if there is no intersection.
         */
        std::optional<GlobeRectangle> intersect(const GlobeRectangle& other) const;

    private:
        double _west;
        double _south;
        double _east;
        double _north;
    };
}
