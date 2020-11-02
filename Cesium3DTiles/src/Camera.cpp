#include "Cesium3DTiles/Camera.h"
#include <glm/trigonometric.hpp>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {

    Camera::Camera(
        const glm::dvec3& position,
        const glm::dvec3& direction,
        const glm::dvec3& up,
        const glm::dvec2& viewportSize,
        double horizontalFieldOfView,
        double verticalFieldOfView,
        const CesiumGeospatial::Ellipsoid& ellipsoid
    ) :
        _position(position),
        _direction(direction),
        _up(up),
        _viewportSize(viewportSize),
        _horizontalFieldOfView(horizontalFieldOfView),
        _verticalFieldOfView(verticalFieldOfView),
        _ellipsoid(ellipsoid),
        _sseDenominator(0.0),
        _positionCartographic(),
        _leftPlane(glm::dvec3(0.0, 0.0, 1.0), 0.0),
        _rightPlane(glm::dvec3(0.0, 0.0, 1.0), 0.0),
        _topPlane(glm::dvec3(0.0, 0.0, 1.0), 0.0),
        _bottomPlane(glm::dvec3(0.0, 0.0, 1.0), 0.0)
    {
        this->updatePositionAndOrientation(position, direction, up);
        this->updateViewParameters(viewportSize, horizontalFieldOfView, verticalFieldOfView);
    }

    void Camera::updatePositionAndOrientation(const glm::dvec3& position, const glm::dvec3& direction, const glm::dvec3& up) {
        this->_position = position;
        this->_direction = direction;
        this->_up = up;

       this->_positionCartographic = this->_ellipsoid.cartesianToCartographic(position);

        this->_updateCullingVolume();
    }

    void Camera::updateViewParameters(const glm::dvec2& viewportSize, double horizontalFieldOfView, double verticalFieldOfView) {
        this->_viewportSize = viewportSize;
        this->_horizontalFieldOfView = horizontalFieldOfView;
        this->_verticalFieldOfView = verticalFieldOfView;

        this->_sseDenominator = 2.0 * glm::tan(0.5 * this->_verticalFieldOfView);

        this->_updateCullingVolume();
    }

    void Camera::_updateCullingVolume() {
        double fovx = this->_horizontalFieldOfView;
        double fovy = this->_verticalFieldOfView;

        double t = glm::tan(0.5 * fovy);
        double b = -t;
        double r = glm::tan(0.5 * fovx);
        double l = -r;

        double n = 1.0;

        // TODO: this is all ported directly from CesiumJS, can probably be refactored to be more efficient with GLM.

        glm::dvec3 right = glm::cross(this->_direction, this->_up);

        glm::dvec3 nearCenter = this->_direction * n;
        nearCenter = this->_position + nearCenter;

        //Left plane computation
        glm::dvec3 normal = right * l;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::normalize(normal);
        normal = glm::cross(normal, this->_up);
        normal = glm::normalize(normal);

        this->_leftPlane = Plane(
            normal,
            -glm::dot(normal, this->_position)
        );

        //Right plane computation
        normal = right * r;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(this->_up, normal);
        normal = glm::normalize(normal);

        this->_rightPlane = Plane(
            normal,
            -glm::dot(normal, this->_position)
        );

        //Bottom plane computation
        normal = this->_up * b;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(right, normal);
        normal = glm::normalize(normal);

        this->_bottomPlane = Plane(
            normal,
            -glm::dot(normal, this->_position)
        );

        //Top plane computation
        normal = this->_up * t;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(normal, right);
        normal = glm::normalize(normal);

        this->_topPlane = Plane(
            normal,
            -glm::dot(normal, this->_position)
        );
    }

    template <class T>
    static bool isBoundingVolumeVisible(
        const T& boundingVolume,
        const Plane& leftPlane,
        const Plane& rightPlane,
        const Plane& bottomPlane,
        const Plane& topPlane
    ) {
        CullingResult left = boundingVolume.intersectPlane(leftPlane);
        if (left == CullingResult::Outside) {
            return false;
        }

        CullingResult right = boundingVolume.intersectPlane(rightPlane);
        if (right == CullingResult::Outside) {
            return false;
        }

        CullingResult top = boundingVolume.intersectPlane(topPlane);
        if (top == CullingResult::Outside) {
            return false;
        }

        CullingResult bottom = boundingVolume.intersectPlane(bottomPlane);
        if (bottom == CullingResult::Outside) {
            return false;
        }

        return true;
    }

    bool Camera::isBoundingVolumeVisible(const BoundingVolume& boundingVolume) const {
        // TODO: use plane masks
        struct Operation {
            const Camera& camera;

            bool operator()(const OrientedBoundingBox& boundingBox) {
                return Cesium3DTiles::isBoundingVolumeVisible(boundingBox, camera._leftPlane, camera._rightPlane, camera._bottomPlane, camera._topPlane);
            }

            bool operator()(const BoundingRegion& boundingRegion) {
                return Cesium3DTiles::isBoundingVolumeVisible(boundingRegion, camera._leftPlane, camera._rightPlane, camera._bottomPlane, camera._topPlane);
            }

            bool operator()(const BoundingSphere& boundingSphere) {
                return Cesium3DTiles::isBoundingVolumeVisible(boundingSphere, camera._leftPlane, camera._rightPlane, camera._bottomPlane, camera._topPlane);
            }

            bool operator()(const BoundingRegionWithLooseFittingHeights& boundingRegion) {
                return Cesium3DTiles::isBoundingVolumeVisible(boundingRegion.getBoundingRegion(), camera._leftPlane, camera._rightPlane, camera._bottomPlane, camera._topPlane);
            }
        };

        return std::visit(Operation { *this }, boundingVolume);
    }

    double Camera::computeDistanceSquaredToBoundingVolume(const BoundingVolume& boundingVolume) const {
        struct Operation {
            const Camera& camera;

            double operator()(const OrientedBoundingBox& boundingBox) {
                return boundingBox.computeDistanceSquaredToPosition(camera._position);
            }

            double operator()(const BoundingRegion& boundingRegion) {
                if (camera._positionCartographic) {
                    return boundingRegion.computeDistanceSquaredToPosition(camera._positionCartographic.value(), camera._position);
                } else {
                    return boundingRegion.computeDistanceSquaredToPosition(camera._position);
                }
            }

            double operator()(const BoundingSphere& boundingSphere) {
                return boundingSphere.computeDistanceSquaredToPosition(camera._position);
            }

            double operator()(const BoundingRegionWithLooseFittingHeights& boundingRegion) {
                if (camera._positionCartographic) {
                    return boundingRegion.computeConservativeDistanceSquaredToPosition(camera._positionCartographic.value(), camera._position);
                } else {
                    return boundingRegion.computeConservativeDistanceSquaredToPosition(camera._position);
                }
            }
        };

        return std::visit(Operation { *this }, boundingVolume);
    }

    double Camera::computeScreenSpaceError(double geometricError, double distance) const {
        // Avoid divide by zero when viewer is inside the tile
        distance = glm::max(distance, 1e-7);
        double sseDenominator = this->_sseDenominator;
        return (geometricError * this->_viewportSize.y) / (distance * sseDenominator);
    }
}
