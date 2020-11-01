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
         * @param westDegrees The westernmost longitude in degrees in the range [-180.0, 180.0].
         * @param southDegrees The southernmost latitude in degrees in the range [-90.0, 90.0].
         * @param eastDegrees The easternmost longitude in degrees in the range [-180.0, 180.0].
         * @param northDegrees The northernmost latitude in degrees in the range [-90.0, 90.0].
         * @returns The rectangle.
         *
         * @snippet TestGlobeRectangle.cpp fromDegrees
         */
        static GlobeRectangle fromDegrees(double westDegrees, double southDegrees, double eastDegrees, double northDegrees);

        /**
         * @brief Returns the westernmost longitude, in radians.
         */
        double getWest() const { return this->_west; }

        /**
         * @brief Returns the southernmost latitude, in radians.
         */
        double getSouth() const { return this->_south; }

        /**
         * @brief Returns the easternmost longitude, in radians.
         */
        double getEast() const { return this->_east; }

        /**
         * @brief Returns the northernmost latitude, in radians.
         */
        double getNorth() const { return this->_north; }

        /**
         * @brief Returns the {@link Cartographic} position of the south-west corner.
         */
        Cartographic getSouthwest() const { return Cartographic(this->_west, this->_south); }

        /**
         * @brief Returns the {@link Cartographic} position of the south-east corner.
         */
        Cartographic getSoutheast() const { return Cartographic(this->_east, this->_south); }

        /**
         * @brief Returns the {@link Cartographic} position of the north-west corner.
         */
        Cartographic getNorthwest() const { return Cartographic(this->_west, this->_north); }

        /**
         * @brief Returns the {@link Cartographic} position of the north-east corner.
         */
        Cartographic getNortheast() const { return Cartographic(this->_east, this->_north); }

        /**
         * @brief Returns this rectangle as a {@link CesiumGemetry::Rectangle}.
         */
        CesiumGeometry::Rectangle toSimpleRectangle() const;

        /**
         * @brief Computes the width of this rectangle.
         * 
         * The result will be in radians, in the range [0, Pi*2].
         */
        double computeWidth() const;

        /**
         * @brief Computes the height of this rectangle.
         *
         * The result will be in radians, in the range [0, Pi*2].
         */
        double computeHeight() const;

        /**
         * @brief Computes the {@link Cartographic} center position of this rectangle.
         */
        Cartographic computeCenter() const;

        /**
         * @brief Returns `true` if this rectangle contains the given point.
         * 
         * This will take into account the wrapping of the latitude and longitude,
         * and also return `true` when the longitude of the given point is within
         * a very small error margin of the longitudes of this rectangle.
         */
        bool contains(const Cartographic& cartographic) const;

        /**
         * @brief Computes the intersection of two rectangles. 
         * 
         * This function assumes that the rectangle's coordinates are latitude and longitude
         * in radians and produces a correct intersection, taking into account the fact that
         * the same angle can be represented with multiple values as well as the wrapping of longitude at the
         * anti-meridian.  For a simple intersection that ignores these factors and can be used with projected
         * coordinates, see {@link CesiumGeometry::Rectangle::intersect}.
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
