#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/SampleHeightResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeospatial/Cartographic.h>

#include <vector>

namespace CesiumAsync {
class AsyncSystem;
}

namespace Cesium3DTilesSelection {

/**
 * @brief An interface to query heights from a tileset that can do so
 * efficiently without necessarily downloading individual tiles.
 */
class CESIUM3DTILESSELECTION_API ITilesetHeightSampler {
public:
  /**
   * @brief Queries the heights at a list of locations.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param positions The positions at which to query heights. The height field
   * of each {@link CesiumGeospatial::Cartographic} is ignored.
   * @return A future that will be resolved when the heights have been queried.
   */
  virtual CesiumAsync::Future<SampleHeightResult> sampleHeights(
      const CesiumAsync::AsyncSystem& asyncSystem,
      std::vector<CesiumGeospatial::Cartographic>&& positions) = 0;
};

} // namespace Cesium3DTilesSelection
