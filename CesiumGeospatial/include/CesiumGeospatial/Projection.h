#pragma once

#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>

#include <glm/vec2.hpp>

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
 * @return The coordinates of the projected point, in the coordinate system
 * of the given projection.
 */
glm::dvec3
projectPosition(const Projection& projection, const Cartographic& position);

/**
 * @brief Unprojects a position from the globe using the given
 * {@link Projection}.
 *
 * @param projection The projection.
 * @param position The coordinates of the point, in meters.
 * @return The {@link Cartographic} position.
 */
Cartographic
unprojectPosition(const Projection& projection, const glm::dvec3& position);

/**
 * @brief Projects a rectangle on the globe by simply projecting its four
 * corners.
 *
 * This is only accurate when the globe rectangle is still a rectangle after
 * projecting, which is true for {@link WebMercatorProjection} but not
 * necessarily true for other projections.
 *
 * @param projection The projection.
 * @param rectangle The globe rectangle to be projected.
 * @return The projected rectangle.
 */
CesiumGeometry::Rectangle projectRectangleSimple(
    const Projection& projection,
    const GlobeRectangle& rectangle);

/**
 * @brief Unprojects a rectangle to the globe by simply unprojecting its four
 * corners.
 *
 * This is only accurate when the rectangle is still a rectangle after
 * unprojecting, which is true for {@link WebMercatorProjection} but not
 * necessarily true for other projections.
 *
 * @param projection The projection.
 * @param rectangle The rectangle to be unprojected.
 * @return The unprojected rectangle.
 */
GlobeRectangle unprojectRectangleSimple(
    const Projection& projection,
    const CesiumGeometry::Rectangle& rectangle);

/**
 * @brief Projects a bounding region on the globe by simply projecting its
 * eight corners.
 *
 * This is only accurate when the globe box is still a box after
 * projecting, which is true for {@link WebMercatorProjection} but not
 * necessarily true for other projections.
 *
 * @param projection The projection.
 * @param region The bounding region to be projected.
 * @return The projected box.
 */
CesiumGeometry::AxisAlignedBox
projectRegionSimple(const Projection& projection, const BoundingRegion& region);

/**
 * @brief Unprojects a box to the globe by simply unprojecting its eight
 * corners.
 *
 * This is only accurate when the box is still a box after
 * unprojecting, which is true for {@link WebMercatorProjection} but not
 * necessarily true for other projections.
 *
 * @param projection The projection.
 * @param box The box to be unprojected.
 * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
 * @return The unprojected bounding region.
 */
BoundingRegion unprojectRegionSimple(
    const Projection& projection,
    const CesiumGeometry::AxisAlignedBox& box,
    const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

/**
 * @brief Computes the approximate real-world size, in meters, of a given
 * projected rectangle.
 *
 * The returned X component corresponds to the size in the projected X
 * direction, while the returned Y component corresponds to the size in the
 * projected Y direction.
 *
 * @param projection The projection.
 * @param rectangle The projected rectangle to measure.
 * @param maxHeight The maximum height of the geometry inside the rectangle.
 * @param ellipsoid The ellipsoid used to convert longitude and latitude to
 * ellipsoid-centered coordinates.
 * @return The approximate size.
 */
glm::dvec2 computeProjectedRectangleSize(
    const Projection& projection,
    const CesiumGeometry::Rectangle& rectangle,
    double maxHeight,
    const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

/**
 * @brief Obtains the ellipsoid used by a Projection variant.
 */
const Ellipsoid& getProjectionEllipsoid(const Projection& projection);

} // namespace CesiumGeospatial
