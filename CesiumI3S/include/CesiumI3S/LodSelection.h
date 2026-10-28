#pragma once
#include <CesiumI3S/Library.h>

namespace CesiumI3S {

/** @brief LoD selection threshold for a node (lodSelection.cmn.md). */
struct CESIUMI3S_API LodSelection {

  /** @brief Metric used to determine whether a node's content is good enough to
   * render (lodSelection.cmn.md). */
  enum class Metric {
    /** @brief Upper limit for the screen size of the diameter of the node's MBS
       in screen pixels. Used with the mesh pyramid profile. */
    MaxScreenThreshold,
    /** @brief Maximum area of the projected bounding volume on screen in pixel
       squared. */
    MaxScreenThresholdSQ,
    /** @brief Scale of the node's minimum bounding volume. Used by the point
       profile. */
    ScreenSpaceRelative,
    /** @brief Distance from the surface of the node's MBS to the camera. Used
       by the point profile. */
    DistanceRangeFromDefaultCamera,
    /** @brief Estimation of the point density covered by the node. Used by the
       point cloud profile. */
    EffectiveDensity,
    /** @brief Point density threshold used by the PCSL (point cloud) profile.
     */
    DensityThreshold
  };

  /** @brief Metric used for LoD selection. */
  LodSelection::Metric metricType = LodSelection::Metric::MaxScreenThreshold;
  /** @brief Maximum metric value, in the CRS of the vertex coordinates or
   * relative to other constants such as screen size. */
  double maxError = 0.0;
};

} // namespace CesiumI3S
