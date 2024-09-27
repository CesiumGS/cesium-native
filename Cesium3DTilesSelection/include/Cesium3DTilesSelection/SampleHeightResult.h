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
   * @brief The positions and their sampled heights.
   *
   * For each resulting position, its longitude and latitude values will match
   * values from its input. Its height will either be the height sampled from
   * the tileset at that position, or the original input height if the sample
   * was unsuccessful. To determine which, look at the value of
   * {@link SampleHeightResult::sampleSuccess} at the same index.
   */
  std::vector<CesiumGeospatial::Cartographic> positions;

  /**
   * @brief The success of each sample.
   *
   * Each entry specifies whether the height for the position at the
   * corresponding index was successfully sampled. If true, then
   * {@link SampleHeightResult::positions} has a valid height sampled from the
   * tileset at this index. If false, the height could not be sampled, leaving
   * the height in {@link SampleHeightResult::positions} unchanged from the
   * original input height.
   */
  std::vector<bool> sampleSuccess;

  /**
   * @brief Any warnings that occurred while sampling heights.
   */
  std::vector<std::string> warnings;
};

} // namespace Cesium3DTilesSelection
