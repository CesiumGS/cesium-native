#pragma once

#include <CesiumGeospatial/Cartographic.h>

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief The result of sampling heights with
 * {@link Tileset::sampleHeightMostDetailed}.
 */
struct SampleHeightResult {
  /**
   * @brief The positions and sampled heights.
   *
   * The longitudes and latitudes will match the values at the same index in the
   * original input positions. Each height will either be the height sampled
   * from the tileset at that position, or the original input height if the
   * height could not be sampled. To determine which, look at the value of
   * {@link SampleHeightResult::heightSampled} at the same index.
   */
  std::vector<CesiumGeospatial::Cartographic> positions;

  /**
   * @brief Specifies whether the height for the position at this index was
   * sampled successfully. If true, {@link SampleHeightResult::positions} has
   * a valid height sampled from the tileset at this index. If false, the height
   * could not be sampled for the position at this index, and so the height in
   * {@link SampleHeightResult::positions} is unchanged from the original input
   * height.
   */
  std::vector<bool> heightSampled;

  /**
   * @brief Any warnings that occurred while sampling heights.
   */
  std::vector<std::string> warnings;
};

} // namespace Cesium3DTilesSelection
