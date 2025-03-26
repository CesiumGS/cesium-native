#include "ITwinRealityDataContentLoader.h"

#include <Cesium3DTilesSelection/ITwinRealityDataContentLoaderFactory.h>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TilesetContentLoaderResult<TilesetContentLoader>>
ITwinRealityDataContentLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const AuthorizationHeaderChangeListener& /*headerChangeListener*/) {
  return ITwinRealityDataContentLoader::createLoader(
             externals,
             this->_realityDataId,
             this->_iTwinId,
             this->_iTwinAccessToken,
             std::move(this->_tokenRefreshCallback),
             tilesetOptions.ellipsoid)
      .thenImmediately(
          [](TilesetContentLoaderResult<ITwinRealityDataContentLoader>&&
                 result) {
            return TilesetContentLoaderResult<TilesetContentLoader>(
                std::move(result));
          });
}
} // namespace Cesium3DTilesSelection