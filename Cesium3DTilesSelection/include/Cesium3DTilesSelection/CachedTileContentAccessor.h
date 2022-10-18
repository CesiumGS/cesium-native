#pragma once

#include "IPrepareRendererResources.h"
#include "TileLoadResult.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/CachingAssetAccessor.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <memory>
#include <optional>
#include <string>

namespace Cesium3DTilesSelection {

class CachedTileContentAccessor {
private:
  typedef std::pair<std::string, std::string> THeader;

public:
  CachedTileContentAccessor(
      const std::shared_ptr<CesiumAsync::CachingAssetAccessor>&
          pCachingAssetAccessor);

  /**
   * @brief Returns the cached tile content if it exists. Otherwise fetches the
   * tile content from the underlying network asset accessor and loads an
   * in-memory glTF using the provided tile loader callback, returning the
   * result. If we find cached tile content, the TileLoadResult::contentKind
   * will be TileCachedRenderContent.
   *
   * Note: This function does not write to the tile content cache. Once the
   * client is done loading the tile and creating "derived" tile content, the
   * arbitrary, binary client data can be cached by calling
   * cacheClientTileContent(..).
   *
   * @param asyncSystem The async system.
   * @param url The tile url to request from.
   * @param headers Any HTTP headers needed for the tile request.
   * @param tileLoader If the cache entry does not exist or needs to be
   * revalidated, this is the tile loader that will get invoked.
   * @return Either cached or partially loaded tile content.
   */
  CesiumAsync::Future<TileLoadResult> getCachedTileContentOr(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers,
      std::function<TileLoadResult(
          std::shared_ptr<CesiumAsync::IAssetRequest>&&)> tileLoader);

  /**
   * @brief Caches derived tile content created and serialized by the client.
   *
   * @param asyncSystem The async system to use.
   * @param loadResult The result of client-side tile loading. Contains info on
   * what to cache.
   */
  CesiumAsync::Future<void> cacheClientTileContent(
      const CesiumAsync::AsyncSystem& asyncSystem,
      ClientTileLoadResult& loadResult);

private:
  std::shared_ptr<CesiumAsync::CachingAssetAccessor> _pCachingAssetAccessor;
};

} // namespace Cesium3DTilesSelection