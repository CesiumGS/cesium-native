#pragma once

#include "CesiumGeospatial/GeographicProjection.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include <variant>

namespace CesiumGeospatial {

    typedef std::variant<GeographicProjection, WebMercatorProjection> Projection;

    glm::dvec3 projectPosition(const Projection& projection, const Cartographic& position);
    Cartographic unprojectPosition(const Projection& projection, const glm::dvec3& position);

    /**
     * Projects a rectangle on the globe by simply projecting its four corners. This is only accurate when the globe rectangle
     * is still a rectangle after projecting, which is true for {@link WebMercatorProjection} but not necessarily true for
     * other projections.
     * 
     * @param projection The projection.
     * @param rectangle The globe rectangle to be projected.
     * @return The projected rectangle.
     */
    CesiumGeometry::Rectangle projectRectangleSimple(const Projection& projection, const GlobeRectangle& rectangle);

    /**
     * Unprojects a rectangle to the globe by simply unprojecting its four corners. This is only accurate when the rectangle
     * is still a rectangle after unprojecting, which is true for {@link WebMercatorProjection} but not necessarily true for
     * other projections.
     * 
     * @param projection The projection.
     * @param rectangle The rectangle to be unprojected.
     * @return The unprojected rectangle.
     */
    GlobeRectangle unprojectRectangleSimple(const Projection& projection, const CesiumGeometry::Rectangle& rectangle);

    /**
     * Computes a conversion factor that, when multiplied by a difference between two projected coordinate values, yields an
     * approximate distance between them in meters.
     * 
     * @param projection The projection.
     * @param position The position near which to compute the conversion factor.
     * @return The conversion factor.
     */
    double computeApproximateConversionFactorToMetersNearPosition(const Projection& projection, const glm::dvec2& position);

}
