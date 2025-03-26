#include "CesiumIonTilesetLoader.h"

#include <Cesium3DTilesSelection/CesiumIonTilesetContentLoaderFactory.h>

namespace Cesium3DTilesSelection {

CesiumAsync::Future<TilesetContentLoaderResult<TilesetContentLoader>>
CesiumIonTilesetContentLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const AuthorizationHeaderChangeListener& headerChangeListener) {
  return CesiumIonTilesetLoader::createLoader(
             externals,
             tilesetOptions.contentOptions,
             this->_ionAssetID,
             this->_ionAccessToken,
             this->_ionAssetEndpointUrl,
             headerChangeListener,
             tilesetOptions.showCreditsOnScreen,
             tilesetOptions.ellipsoid)
      .thenImmediately([](Cesium3DTilesSelection::TilesetContentLoaderResult<
                           CesiumIonTilesetLoader>&& result) {
        return TilesetContentLoaderResult<TilesetContentLoader>(
            std::move(result));
      });
}
} // namespace Cesium3DTilesSelection