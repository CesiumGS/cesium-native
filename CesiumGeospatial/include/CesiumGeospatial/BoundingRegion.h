#pragma once

#include "CesiumGeospatial/Library.h"
#include "CesiumGeometry/CullingResult.h"
#include "CesiumGeometry/OrientedBoundingBox.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Ellipsoid.h"

namespace CesiumGeometry {
    class Plane;
}

namespace CesiumGeospatial {
    class Cartographic;

    /**
     * @brief A bounding volume specified as a longitude/latitude bounding box and a minimum and maximum height.
     */
    class CESIUMGEOSPATIAL_API BoundingRegion {
    public:
        /**
         * @brief Constructs a new bounding region.
         * 
         * @param rectangle The bounding rectangle of the region.
         * @param minimumHeight The minimum height in meters.
         * @param maximumHeight The maximum height in meters.
         * @param ellipsoid The ellipsoid on which this region is defined.
         */
        BoundingRegion(
            const GlobeRectangle& rectangle,
            double minimumHeight,
            double maximumHeight,
            const Ellipsoid& ellipsoid = Ellipsoid::WGS84
        );

        /**
         * @brief Gets the bounding rectangle of the region.
         */
        const GlobeRectangle& getRectangle() const { return this->_rectangle; }

        /**
         * @brief Gets the minimum height of the region.
         */
        double getMinimumHeight() const { return this->_minimumHeight; }

        /**
         * @brief Gets the maximum height of the region.
         */
        double getMaximumHeight() const { return this->_maximumHeight; }

        /**
         * @brief Gets an oriented bounding box containing this region.
         */
        const CesiumGeometry::OrientedBoundingBox& getBoundingBox() const { return this->_boundingBox; }

        /**
         * @brief Determines on which side of a plane the bounding region is located.
         * 
         * @param plane The plane to test against.
         * @return The {@link CesiumGeometry::CullingResult} 
         *  * `Inside` if the entire region is on the side of the plane the normal is pointing.
         *  * `Outside` if the entire region is on the opposite side.
         *  * `Intersecting` if the region intersects the plane.
         */
        CesiumGeometry::CullingResult intersectPlane(const CesiumGeometry::Plane& plane) const;

        /**
         * @brief Computes the distance-squared from a position in ellipsoid-centered Cartesian coordinates
         * to the closest point in this bounding region.
         * 
         * @param position The position.
         * @param ellipsoid The ellipsoid on which this region is defined.
         * @return The distance-squared from the position to the closest point in the bounding region.
         */
        double computeDistanceSquaredToPosition(const glm::dvec3& position, const Ellipsoid& ellipsoid = Ellipsoid::WGS84) const;

        /**
         * @brief Computes the distance-squared from a longitude-latitude-height position to the closest point in this bounding region.
         * 
         * @param position The position.
         * @param ellipsoid The ellipsoid on which this region is defined.
         * @return The distance-squared from the position to the closest point in the bounding region.
         */
        double computeDistanceSquaredToPosition(const Cartographic& position, const Ellipsoid& ellipsoid = Ellipsoid::WGS84) const;

        /**
         * @brief Computes the distance-squared from a position to the closest point in this bounding region, when the longitude-latitude-height
         * and ellipsoid-centered Cartesian coordinates of the position are both already known.
         * 
         * @param cartographicPosition The position as a longitude-latitude-height.
         * @param cartesianPosition The position as ellipsoid-centered Cartesian coordinates.
         * @return The distance-squared from the position to the closest point in the bounding region.
         */
        double computeDistanceSquaredToPosition(const Cartographic& cartographicPosition, const glm::dvec3& cartesianPosition) const;

private:
        static CesiumGeometry::OrientedBoundingBox _computeBoundingBox(
            const GlobeRectangle& rectangle,
            double minimumHeight,
            double maximumHeight,
            const Ellipsoid& ellipsoid);

        GlobeRectangle _rectangle;
        double _minimumHeight;
        double _maximumHeight;
        CesiumGeometry::OrientedBoundingBox _boundingBox;
        glm::dvec3 _southwestCornerCartesian;
        glm::dvec3 _northeastCornerCartesian;
        glm::dvec3 _westNormal;
        glm::dvec3 _eastNormal;
        glm::dvec3 _southNormal;
        glm::dvec3 _northNormal;
    };

}
