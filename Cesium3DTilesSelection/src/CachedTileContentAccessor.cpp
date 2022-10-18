#include <Cesium3DTilesSelection/CachedTileContentAccessor.h>

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

CachedTileContentAccessor::CachedTileContentAccessor(
    const std::shared_ptr<CesiumAsync::CachingAssetAccessor>&
        pCachingAssetAccessor)
    : _pCachingAssetAccessor(pCachingAssetAccessor) {}

Future<TileLoadResult> CachedTileContentAccessor::getCachedTileContentOr(
    const AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers,
    std::function<TileLoadResult(std::shared_ptr<IAssetRequest>&&)>
        tileLoader) {
  // Check for cached tile content. We disable write-through of newly fetched
  // network responses so that we can manually write back to the cache later
  // with derived client data.
  return this->_pCachingAssetAccessor->get(asyncSystem, url, headers, false)
      .thenInWorkerThread(
          [tileLoader](std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
            // If the request is of CacheAssetRequest type, then it came from
            // the cache asset accessor. Otherwise the underlying network asset
            // accessor was used and we need to write back to cache later.
            CacheAssetRequest* pCacheHit =
                dynamic_cast<CacheAssetRequest*>(pCompletedRequest.get());
            if (pCacheHit) {
              return TileLoadResult::createCacheHitResult(
                  std::move(pCompletedRequest));
            } else {
              return tileLoader(std::move(pCompletedRequest));
            }
          });
}

Future<void> CachedTileContentAccessor::cacheClientTileContent(
    const AsyncSystem& asyncSystem,
    ClientTileLoadResult& loadResult) {
  return this->_pCachingAssetAccessor->writeBack(
      asyncSystem,
      loadResult.result.pCompletedRequest,
      loadResult.cacheOriginalResponseData,
      std::move(loadResult.clientDataToCache));
}
} // namespace Cesium3DTilesSelection