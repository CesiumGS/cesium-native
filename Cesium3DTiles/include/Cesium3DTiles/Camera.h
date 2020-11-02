#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "CesiumGeometry/Plane.h"
#include "CesiumGeospatial/Cartographic.h"
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace Cesium3DTiles {

    /**
     * @brief A camera in 3D space.
     *
     * A camera is defined by a position, orientation and the view frustum.
     */
    class CESIUM3DTILES_API Camera {
    public:
        // TODO: Add support for orthographic and off-center perspective frustums

        Camera(
            const glm::dvec3& position,
            const glm::dvec3& direction,
            const glm::dvec3& up,
            const glm::dvec2& viewportSize,
            double horizontalFieldOfView,
            double verticalFieldOfView,
            const CesiumGeospatial::Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84
        );

        /**
         * @brief Gets the position of the camera in Earth-centered, Earth-fixed coordinates.
         */
        const glm::dvec3& getPosition() const { return this->_position; }

        /**
         * @brief Gets the look direction of the camera in Earth-centered, Earth-fixed coordinates.
         */
        const glm::dvec3& getDirection() const { return this->_direction; }

        /**
         * @brief Gets the up direction of the camera in Earth-centered, Earth-fixed coordinates.
         */
        const glm::dvec3& getUp() const { return this->_up; }

        /**
         * @brief Gets the position of the camera as a longitude / latitude / height.
         * 
         * The result may be `std::nullopt` if the Cartesian position is
         * very near the center of the Ellipsoid.
         */
        const std::optional<CesiumGeospatial::Cartographic>& getPositionCartographic() const { return this->_positionCartographic; }

        /**
         * @brief Gets the size of the viewport in pixels.
         */
        const glm::dvec2& getViewportSize() const { return this->_viewportSize; }

        /**
         * @brief Gets the horizontal field-of-view angle in radians.
         */
        double getHorizontalFieldOfView() const { return this->_horizontalFieldOfView; }

        /**
         * @brief Gets the vertical field-of-view angle in radians.
         */
        double getVerticalFieldOfView() const { return this->_verticalFieldOfView; }

        /**
         * @brief Gets the denominator used in screen-space error (SSE) computations.
         * 
         * The denominator is <code>2.0 * tan(0.5 * verticalFieldOfView)</code>
         */
        double getScreenSpaceErrorDenominator() const { return this->_sseDenominator; }

        /**
         * @brief Updates the position and orientation of the camera.
         * 
         * @param position The new position
         * @param direction The new look direction vector
         * @param up The new up vector
         */
        void updatePositionAndOrientation(const glm::dvec3& position, const glm::dvec3& direction, const glm::dvec3& up);

        /**
         * @brief Updates the camera's view parameters.
         *
         * @param viewportSize The new size of the viewport, in pixels
         * @param horizontalFieldOfView The horizontal field of view angle in radians.
         * @param verticalFieldOfView The vertical field of view angle in radians.
         */
        void updateViewParameters(const glm::dvec2& viewportSize, double horizontalFieldOfView, double verticalFieldOfView);

        /**
         * @brief Returns whether the given {@link BoundingVolume} is visible for this camera
         *
         * Returns whether the given bounding volume is completely visible,
         * meaning that all four planes of the given volume are completely
         * contained in the frustum of this camera.
         *
         * @return Whether the bounding volume is visible
         */
        bool isBoundingVolumeVisible(const BoundingVolume& boundingVolume) const;

        /**
         * @brief Computes the squared distance to the given {@link BoundingVolume}.
         * 
         * Computes the squared euclidean distance from the position of this camera
         * to the closest point of the given bounding volume.
         * 
         * @param boundingVolume The bounding volume
         * @returns The squared distance
         */
        double computeDistanceSquaredToBoundingVolume(const BoundingVolume& boundingVolume) const;

        /**
         * @brief Computes the screen space error from a given geometric error
         *
         * Computes the screen space error (SSE) that results from the given
         * geometric error, when it is viewed with this camera from the given
         * distance.
         *
         * The given distance will be clamped to a small positive value if
         * it is negative or too close to zero.
         *
         * @param geometricError The geometric error
         * @param distance The viewing distance
         * @return The screen space error
         */
        double computeScreenSpaceError(double geometricError, double distance) const;

    private:
        void _updateCullingVolume();

        glm::dvec3 _position;
        glm::dvec3 _direction;
        glm::dvec3 _up;
        glm::dvec2 _viewportSize;
        double _horizontalFieldOfView;
        double _verticalFieldOfView;
        CesiumGeospatial::Ellipsoid _ellipsoid;

        double _sseDenominator;
        std::optional<CesiumGeospatial::Cartographic> _positionCartographic;

        CesiumGeometry::Plane _leftPlane;
        CesiumGeometry::Plane _rightPlane;
        CesiumGeometry::Plane _topPlane;
        CesiumGeometry::Plane _bottomPlane;
    };

}
