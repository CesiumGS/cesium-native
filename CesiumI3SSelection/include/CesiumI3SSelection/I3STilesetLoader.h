#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/EarthGravitationalModel1996Grid.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumI3S/Node.h>
#include <CesiumI3S/Scene.h>
#include <CesiumI3SSelection/IResourceLocator.h>
#include <CesiumI3SSelection/Library.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace spdlog {
class logger;
}

namespace CesiumI3SSelection {

/**
 * @brief Loads an I3S scene layer as a stream of 3D Tiles.
 *
 * Node indices are stored as decimal `std::string` TileIDs.
 */
class CESIUMI3SSELECTION_API I3STilesetLoader
    : public Cesium3DTilesSelection::TilesetContentLoader {
public:
  /**
   * @brief Asynchronously fetches the layer JSON at \p layerUrl and constructs
   * an I3STilesetLoader along with its root tile.
   */
  static CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<I3STilesetLoader>>
  createLoader(
      const Cesium3DTilesSelection::TilesetExternals& externals,
      std::shared_ptr<IResourceLocator> pResourceLocator,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const std::shared_ptr<CesiumGeospatial::EarthGravitationalModel1996Grid>&
          pGeoidGrid = nullptr);

  CesiumAsync::Future<Cesium3DTilesSelection::TileLoadResult>
  loadTileContent(const Cesium3DTilesSelection::TileLoadInput& input) override;

  Cesium3DTilesSelection::TileChildrenResult createTileChildren(
      const Cesium3DTilesSelection::Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) override;

  void onTileContentUnloaded(
      const Cesium3DTilesSelection::Tile& tile) noexcept override;

private:
  I3STilesetLoader(
      std::shared_ptr<IResourceLocator> pResourceLocator,
      CesiumI3S::Layer layer,
      std::shared_ptr<CesiumGeospatial::EarthGravitationalModel1996Grid>
          pGeoidGrid);

  uint32_t
  selectGeometryBufferIndex(uint32_t geometryDefinitionIndex) const noexcept;
  Cesium3DTilesSelection::BoundingVolume nodeObbToEcef(
      const CesiumI3S::OrientedBoundingBox& obb,
      const CesiumGeospatial::Ellipsoid& ellipsoid) const noexcept;

  CesiumAsync::Future<bool> ensureNodePageCached(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      uint32_t nodeIndex);

  CesiumAsync::Future<bool> fetchAndCacheNodePage(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      uint32_t pageIndex);

  CesiumAsync::Future<bool> ensureChildPagesCached(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const CesiumI3S::Node& node);

  std::shared_ptr<CesiumI3S::NodePage>
  getNodePageFromCache(uint32_t pageIndex) const noexcept;
  const CesiumI3S::Node* getNodeFromCache(uint32_t nodeIndex) const noexcept;

  std::shared_ptr<IResourceLocator> _pResourceLocator;
  CesiumI3S::Layer _layer;
  std::shared_ptr<CesiumGeospatial::EarthGravitationalModel1996Grid>
      _pGeoidGrid;

  // Parsed node pages keyed by page index.  shared_ptr values allow worker
  // threads to safely hold a reference while the main thread may evict entries.
  mutable std::mutex _nodePageCacheMutex;
  std::unordered_map<uint32_t, std::shared_ptr<CesiumI3S::NodePage>>
      _nodePageCache;
  // Ref-counts how many live tiles were loaded from each page.  When a tile is
  // unloaded via onTileContentUnloaded the count is decremented; reaching zero
  // evicts the page from _nodePageCache.
  std::unordered_map<uint32_t, uint32_t> _pageRefCounts;
};

} // namespace CesiumI3SSelection
