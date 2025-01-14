#pragma once

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Library.h>

namespace CesiumGeospatial {

/**
 * @brief Helper class for creating a \ref BoundingRegion or \ref GlobeRectangle
 * from a set of points.
 */
class CESIUMGEOSPATIAL_API BoundingRegionBuilder {
public:
  /**
   * @brief Constructs a new instance, starting with an empty bounding region.
   */
  BoundingRegionBuilder() noexcept;

  /**
   * @brief Gets the final region from this builder.
   *
   * If no positions are added to this builder, the returned region's rectangle
   * will be {@link GlobeRectangle::EMPTY}, its minimum height will be 1.0, and
   * its maximum height will be -1.0 (the minimum will be greater than the
   * maximum).
   */
  BoundingRegion
  toRegion(const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID) const;

  /**
   * @brief Gets the final globe rectangle from this builder.
   *
   * If no positions are added to this builder, the returned region's rectangle
   * will be {@link GlobeRectangle::EMPTY}.
   */
  GlobeRectangle toGlobeRectangle() const;

  /**
   * @brief Sets the distance from the North or South pole, in radians, that is
   * considered "too close" to rely on the longitude value.
   *
   * When a position given to {@link expandToIncludePosition} has a latitude closer than this value,
   * the region will be updated to include the position's _latitude_, but the
   * position's longitude will be ignored.
   *
   * @param tolerance The new tolerance.
   */
  void setPoleTolerance(double tolerance) noexcept;

  /**
   * @brief Gets the distance from the North or South pole, in radians, that is
   * considered "too close" to rely on the longitude value.
   *
   * When a position given to {@link expandToIncludePosition} has a latitude closer than this value,
   * the region will be updated to include the position's _latitude_, but the
   * position's longitude will be ignored.
   *
   * @return The tolerance.
   */
  double getPoleTolerance() const noexcept;

  /**
   * @brief Expands the bounding region to include the given position.
   *
   * The region will be kept as small as possible.
   *
   * @param position The position to be included in the region.
   * @returns True if the region was modified, or false if the region already
   * contained the position.
   */
  bool expandToIncludePosition(const Cartographic& position);

private:
  /**
   * @brief When a position's latitude is within this distance in radians from
   * the North or South pole, its longitude should be considered unreliable and
   * not used to expand the bounding region in that direction.
   */
  double _poleTolerance;
  GlobeRectangle _rectangle;
  double _minimumHeight;
  double _maximumHeight;

  /**
   * @brief True if the region is empty (covers no space) in the longitude
   * direction.
   *
   * We know the rectangle is empty in the latitude direction when South
   * is greater than North. But due to wrapping at the anti-meridian, the West
   * longitude may very well be greater than the East longitude. So this field
   * explicitly tracks whether the region is empty in the longitude direction.
   *
   * It's possible for the longitude range to be empty while the latitude range
   * is not when all of the points used to expand the region so far have been
   * too close to the poles to reliably use to compute longitude.
   */
  bool _longitudeRangeIsEmpty;
};

} // namespace CesiumGeospatial
