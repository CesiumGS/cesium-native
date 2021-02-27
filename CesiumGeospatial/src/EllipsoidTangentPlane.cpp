// Copyright CesiumGS, Inc. and Contributors

#include "CesiumGeospatial/EllipsoidTangentPlane.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGeometry/Plane.h"
#include "CesiumGeometry/Ray.h"
#include "CesiumGeometry/IntersectionTests.h"

#include <stdexcept>

using namespace CesiumGeometry;

namespace CesiumGeospatial {

    EllipsoidTangentPlane::EllipsoidTangentPlane(
        const glm::dvec3& origin,
        const Ellipsoid& ellipsoid
    ) :
        EllipsoidTangentPlane(
            computeEastNorthUpToFixedFrame(origin, ellipsoid),
            ellipsoid)
    {
    }

    EllipsoidTangentPlane::EllipsoidTangentPlane(
        const glm::dmat4& eastNorthUpToFixedFrame,
        const Ellipsoid& ellipsoid
    ) :
        _ellipsoid(ellipsoid),
        _origin(eastNorthUpToFixedFrame[3]),
        _xAxis(eastNorthUpToFixedFrame[0]),
        _yAxis(eastNorthUpToFixedFrame[1]),
        _plane(eastNorthUpToFixedFrame[3], eastNorthUpToFixedFrame[2])
    {
    }

    glm::dvec2 EllipsoidTangentPlane::projectPointToNearestOnPlane(const glm::dvec3& cartesian) const noexcept {
        Ray ray(cartesian, this->_plane.getNormal());

        std::optional<glm::dvec3> intersectionPoint = IntersectionTests::rayPlane(ray, this->_plane);
        if (!intersectionPoint) {
            intersectionPoint = IntersectionTests::rayPlane(-ray, this->_plane);
            if (!intersectionPoint) {
                intersectionPoint = cartesian;
            }
        }

        glm::dvec3 v = intersectionPoint.value() - this->_origin;
        return glm::dvec2(
            glm::dot(this->_xAxis, v),
            glm::dot(this->_yAxis, v)
        );
    }

    /* static */ glm::dmat4 EllipsoidTangentPlane::computeEastNorthUpToFixedFrame(
        const glm::dvec3& origin,
        const Ellipsoid& ellipsoid) {
        const auto scaledOrigin = ellipsoid.scaleToGeodeticSurface(origin);
        if (!scaledOrigin) {
            throw std::invalid_argument("The origin must not be near the center of the ellipsoid.");
        }
        return Transforms::eastNorthUpToFixedFrame(
            scaledOrigin.value(), ellipsoid);
    }




}
