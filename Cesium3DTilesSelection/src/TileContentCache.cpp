#include <Cesium3DTilesSelection/TileContentCache.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

TileContentCache::TileContentCache(
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor)
    : _pAssetAccessor(pAssetAccessor) {
  assert(pAssetAccessor != nullptr);
}

Future<TileLoadResult> TileContentCache::getOrLoad(
    const AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers,
    std::function<TileLoadResult(std::shared_ptr<IAssetRequest>&&)>
        loadTileContentInWorkerThread) {

  // Check for cached tile content. We disable write-through of newly fetched
  // network responses so that we can manually write back to the cache later
  // with derived client data.
  return this->_pAssetAccessor->get(asyncSystem, url, headers, false)
      .thenInWorkerThread(
          [loadTileContentInWorkerThread =
               std::move(loadTileContentInWorkerThread)](
              std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
            assert(pCompletedRequest != nullptr);

            const IAssetResponse* pResponse = pCompletedRequest->response();
            if (pResponse && !pResponse->clientData().empty()) {
              return TileLoadResult::createCacheHitResult(
                  std::move(pCompletedRequest));
            }

            // If no client data was stored, try loading from the response
            // data.

            return loadTileContentInWorkerThread(std::move(pCompletedRequest));
          });
}

Future<void> TileContentCache::store(
    const AsyncSystem& asyncSystem,
    ClientTileLoadResult& loadResult) {
  return this->_pAssetAccessor->writeBack(
      asyncSystem,
      loadResult.result.pCompletedRequest,
      loadResult.cacheOriginalResponseData,
      std::move(loadResult.clientDataToCache));
}
} // namespace Cesium3DTilesSelection