#pragma once

// TileOverlaySystem.h lives in src/ (internal) because RasterOverlayUpsampler
// is also internal. If RasterOverlayUpsampler is ever promoted to a public
// header this file can move to include/.

#include "RasterOverlayUpsampler.h"

#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumGeospatial/Ellipsoid.h>

namespace Cesium3DTilesSelection {

/**
 * @brief Owns the raster overlay collection and the upsampler.
 *
 * This is the single point of ownership for all overlay-related resources.
 * In the current design tiles hold references into data owned here.
 *
 * The upsampler is a `TilesetContentLoader` specialisation whose `setOwner()`
 * must be called by the containing `TilesetContentManager` after construction,
 * because the owner back-reference is to the manager, not the system.
 *
 * [main-thread-only]
 */
class TileOverlaySystem {
public:
  TileOverlaySystem(
      const LoadedTileEnumerator& loadedTiles,
      const TilesetExternals& externals,
      const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept;

  ~TileOverlaySystem() noexcept = default;

  TileOverlaySystem(const TileOverlaySystem&) = delete;
  TileOverlaySystem& operator=(const TileOverlaySystem&) = delete;
  TileOverlaySystem(TileOverlaySystem&&) noexcept = default;
  TileOverlaySystem& operator=(TileOverlaySystem&&) noexcept = default;

  /** @brief The raster overlay collection. [main-thread] */
  const RasterOverlayCollection& getCollection() const noexcept {
    return _collection;
  }
  RasterOverlayCollection& getCollection() noexcept { return _collection; }

  /**
   * @brief The quadtree-upsampling loader.
   * Caller is responsible for calling `setOwner()` after construction.
   * [main-thread]
   */
  const RasterOverlayUpsampler& getUpsampler() const noexcept {
    return _upsampler;
  }
  RasterOverlayUpsampler& getUpsampler() noexcept { return _upsampler; }

private:
  RasterOverlayCollection _collection;
  RasterOverlayUpsampler _upsampler;
};

} // namespace Cesium3DTilesSelection
