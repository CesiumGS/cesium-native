#pragma once

#include "IPrepareRendererResources.h"
#include "TileLoadResult.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <memory>
#include <optional>
#include <string>

namespace Cesium3DTilesSelection {

class TileContentCache {
private:
  typedef std::pair<std::string, std::string> THeader;

public:
  /**
   * @brief Create a tile content cache with the given asset accessor.
   *
   * @param pAssetAccessor The underlying asset accessor around which to
   * build the tile content cache.
   */
  TileContentCache(
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor);

  /**
   * @brief Returns the cached tile content if it exists. Otherwise fetches a
   * response from the underlying network asset accessor and loads an in-memory
   * glTF using the provided tile loader callback, returning the result. If we
   * find cached tile content, the TileLoadResult::contentKind will be
   * TileCachedRenderContent.
   *
   * Note: This function does not write to the tile content cache. Once the
   * client is done loading the tile and creating "derived" tile content, the
   * arbitrary, binary client data can be written back to cache by calling
   * store(..).
   *
   * @param asyncSystem The async system.
   * @param url The tile url to request from.
   * @param headers Any HTTP headers needed for the tile request.
   * @param loadTileContentInWorkerThread If the cache entry does not exist or
   * needs to be revalidated, this tile loader callback will be invoked.
   * @return Either cached or partially loaded tile content.
   */
  CesiumAsync::Future<TileLoadResult> getOrLoad(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers,
      std::function<
          TileLoadResult(std::shared_ptr<CesiumAsync::IAssetRequest>&&)>
          loadTileContentInWorkerThread);

  /**
   * @brief Caches derived tile content created and serialized by the client.
   *
   * @param asyncSystem The async system to use.
   * @param loadResult The result of client-side tile loading. Contains info on
   * what to cache.
   */
  CesiumAsync::Future<void> store(
      const CesiumAsync::AsyncSystem& asyncSystem,
      ClientTileLoadResult& loadResult);

private:
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
};

} // namespace Cesium3DTilesSelection