#pragma once

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Library.h>

namespace CesiumGeospatial {

/**
 * @brief A {@link BoundingRegion} whose heights might be very inaccurate and so
 * distances should be estimated conservatively for level-of-detail
 * computations.
 *
 * An instance of this class serves as a marker of the imprecision of the
 * heights in a {@link BoundingRegion}, and also has a
 * {@link BoundingRegionWithLooseFittingHeights::computeConservativeDistanceSquaredToPosition}
 * method to compute the conservative distance metric.
 */
class CESIUMGEOSPATIAL_API BoundingRegionWithLooseFittingHeights final {
public:
  /**
   * @brief Constructs a new bounding region.
   *
   * @param boundingRegion The bounding region that has imprecise heights.
   */
  BoundingRegionWithLooseFittingHeights(
      const BoundingRegion& boundingRegion) noexcept;

  /**
   * @brief Gets the bounding region that has imprecise heights.
   */
  const BoundingRegion& getBoundingRegion() const noexcept {
    return this->_region;
  }

  /**
   * @brief Computes the conservative distance-squared from a position in
   * ellipsoid-centered Cartesian coordinates to the closest point in this
   * bounding region.
   *
   * It is conservative in that the distance is computed using whichever
   * is _farther away_  of this bounding region's imprecise minimum and maximum
   * heights, so the returned distance may be greater than what the distance to
   * the bounding region would be if the heights were precise. When used for
   * level-of-detail selection, this ensures that imprecise selection caused by
   * the imprecise heights will cause _too little_ detail to be loaded rather
   * than too much detail. This is important because overestimating the required
   * level-of-detail can require an excessive number of tiles to be loaded.
   *
   * @param position The position.
   * @param ellipsoid The ellipsoid on which this region is defined.
   * @return The distance-squared from the position to the closest point in the
   * bounding region.
   */
  double computeConservativeDistanceSquaredToPosition(
      const glm::dvec3& position,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID) const noexcept;

  /**
   * @brief Computes the conservative distance-squared from a
   * longitude-latitude-height position to the closest point in this bounding
   * region.
   *
   * It is conservative in that the distance is computed using whichever
   * is _farther away_  of this bounding region's imprecise minimum and maximum
   * heights, so the returned distance may be greater than what the distance to
   * the bounding region would be if the heights were precise. When used for
   * level-of-detail selection, this ensures that imprecise selection caused by
   * the imprecise heights will cause _too little_ detail to be loaded rather
   * than too much detail. This is important because overestimating the required
   * level-of-detail can require an excessive number of tiles to be loaded.
   *
   * @param position The position.
   * @param ellipsoid The ellipsoid on which this region is defined.
   * @return The distance-squared from the position to the closest point in the
   * bounding region.
   */
  double computeConservativeDistanceSquaredToPosition(
      const Cartographic& position,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID) const noexcept;

  /**
   * @brief Computes the conservative distance-squared from a position to the
   * closest point in this bounding region, when the longitude-latitude-height
   * and ellipsoid-centered Cartesian coordinates of the position are both
   * already known.
   *
   * It is conservative in that the distance is computed using whichever
   * is _farther away_  of this bounding region's imprecise minimum and maximum
   * heights, so the returned distance may be greater than what the distance to
   * the bounding region would be if the heights were precise. When used for
   * level-of-detail selection, this ensures that imprecise selection caused by
   * the imprecise heights will cause _too little_ detail to be loaded rather
   * than too much detail. This is important because overestimating the required
   * level-of-detail can require an excessive number of tiles to be loaded.
   *
   * @param cartographicPosition The position as a longitude-latitude-height.
   * @param cartesianPosition The position as ellipsoid-centered Cartesian
   * coordinates.
   * @return The distance-squared from the position to the closest point in the
   * bounding region.
   */
  double computeConservativeDistanceSquaredToPosition(
      const Cartographic& cartographicPosition,
      const glm::dvec3& cartesianPosition) const noexcept;

private:
  BoundingRegion _region;
};

} // namespace CesiumGeospatial
