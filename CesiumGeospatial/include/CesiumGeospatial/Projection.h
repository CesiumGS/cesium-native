#pragma once

#include "CesiumGeospatial/GeographicProjection.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include <variant>

namespace CesiumGeospatial {

    /**
     * @brief A projection.
     * 
     * This is a `std::variant` of different types of map projections that
     * can convert between projected map coordinates and Cartographic coordinates.
     * 
     * @see GeographicProjection
     * @see WebMercatorProjection
     */
    typedef std::variant<GeographicProjection, WebMercatorProjection> Projection;

    /**
     * @brief Projects a position on the globe using the given {@link Projection}.
     * 
     * @param projection The projection.
     * @param position The {@link Cartographic} position.
     * @return The coordinates of the projected point, in meters.
     */
    glm::dvec3 projectPosition(const Projection& projection, const Cartographic& position);

    /**
     * @brief Unprojects a position from the globe using the given {@link Projection}.
     *
     * @param projection The projection.
     * @param position The coordinates of the point, in meters.
     * @return The {@link Cartographic} position.
     */
    Cartographic unprojectPosition(const Projection& projection, const glm::dvec3& position);

    /**
     * @brief Projects a rectangle on the globe by simply projecting its four corners. 
     *
     * This is only accurate when the globe rectangle is still a rectangle after projecting, which is 
     * true for {@link WebMercatorProjection} but not necessarily true for other projections.
     * 
     * @param projection The projection.
     * @param rectangle The globe rectangle to be projected.
     * @return The projected rectangle.
     */
    CesiumGeometry::Rectangle projectRectangleSimple(const Projection& projection, const GlobeRectangle& rectangle);

    /**
     * @brief Unprojects a rectangle to the globe by simply unprojecting its four corners. 
     * 
     * This is only accurate when the rectangle is still a rectangle after unprojecting, which is 
     * true for {@link WebMercatorProjection} but not necessarily true for other projections.
     * 
     * @param projection The projection.
     * @param rectangle The rectangle to be unprojected.
     * @return The unprojected rectangle.
     */
    GlobeRectangle unprojectRectangleSimple(const Projection& projection, const CesiumGeometry::Rectangle& rectangle);

    /**
     * @brief Computes a factor for distance approximations.
     *
     * Computes a conversion factor that, when multiplied by a difference between two projected coordinate values, yields an
     * approximate distance between them in meters.
     * 
     * @param projection The projection.
     * @param position The position near which to compute the conversion factor.
     * @return The conversion factor.
     */
    double computeApproximateConversionFactorToMetersNearPosition(const Projection& projection, const glm::dvec2& position);

}
