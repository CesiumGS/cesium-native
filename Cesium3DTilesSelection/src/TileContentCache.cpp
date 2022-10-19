#include <Cesium3DTilesSelection/TileContentCache.h>

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

TileContentCache::TileContentCache(
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor)
    : _pAssetAccessor(pAssetAccessor),
      _pCachingAssetAccessor(
          dynamic_cast<CachingAssetAccessor*>(pAssetAccessor.get())) {
  assert(pAssetAccessor != nullptr);
}

Future<TileLoadResult> TileContentCache::getOrLoad(
    const AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers,
    std::function<TileLoadResult(std::shared_ptr<IAssetRequest>&&)>
        loadTileContentInWorkerThread) {
  if (!this->_pCachingAssetAccessor) {
    return this->_pAssetAccessor->get(asyncSystem, url, headers)
        .thenInWorkerThread(
            [loadTileContentInWorkerThread =
                 std::move(loadTileContentInWorkerThread)](
                std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
              return loadTileContentInWorkerThread(
                  std::move(pCompletedRequest));
            });
  }

  // Check for cached tile content. We disable write-through of newly fetched
  // network responses so that we can manually write back to the cache later
  // with derived client data.
  return this->_pCachingAssetAccessor->get(asyncSystem, url, headers, false)
      .thenInWorkerThread(
          [loadTileContentInWorkerThread =
               std::move(loadTileContentInWorkerThread)](
              std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
            // If the request is of CacheAssetRequest type, then it came from
            // the cache asset accessor (it was a cache hit). Otherwise the
            // underlying network asset accessor was used and we need to write
            // back to cache later.
            CacheAssetRequest* pCacheHit =
                dynamic_cast<CacheAssetRequest*>(pCompletedRequest.get());
            if (pCacheHit) {
              // If the client previously stored any custom data, assume it is
              // sufficient for the client to re-load this tiles content.
              const CacheAssetResponse* pCacheResponse =
                  dynamic_cast<const CacheAssetResponse*>(
                      pCacheHit->response());
              if (!pCacheResponse->clientData().empty()) {
                return TileLoadResult::createCacheHitResult(
                    std::move(pCompletedRequest));
              }

              // If no client data was stored, try loading from the response
              // data.
            }

            return loadTileContentInWorkerThread(std::move(pCompletedRequest));
          });
}

Future<void> TileContentCache::store(
    const AsyncSystem& asyncSystem,
    ClientTileLoadResult& loadResult) {
  if (!this->_pCachingAssetAccessor) {
    return asyncSystem.createResolvedFuture();
  }

  return this->_pCachingAssetAccessor->writeBack(
      asyncSystem,
      loadResult.result.pCompletedRequest,
      loadResult.cacheOriginalResponseData,
      std::move(loadResult.clientDataToCache));
}
} // namespace Cesium3DTilesSelection