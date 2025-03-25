#include "ITwinRealityDataContentLoader.h"

#include <Cesium3DTilesSelection/ITwinRealityDataContentLoaderFactory.h>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TilesetContentLoaderResult<TilesetContentLoader>>
ITwinRealityDataContentLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const AuthorizationHeaderChangeListener& /*headerChangeListener*/) const {
  return ITwinRealityDataContentLoader::createLoader(
             externals,
             this->_realityDataId,
             this->_iTwinId,
             this->_iTwinAccessToken,
             tilesetOptions.ellipsoid)
      .thenImmediately(
          [](TilesetContentLoaderResult<ITwinRealityDataContentLoader>&&
                 result) {
            return TilesetContentLoaderResult<TilesetContentLoader>(
                std::move(result));
          });
}
} // namespace Cesium3DTilesSelection