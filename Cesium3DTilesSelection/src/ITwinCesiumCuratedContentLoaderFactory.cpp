#include "ITwinCesiumCuratedContentLoader.h"

#include <Cesium3DTilesSelection/ITwinCesiumCuratedContentLoaderFactory.h>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TilesetContentLoaderResult<TilesetContentLoader>>
ITwinCesiumCuratedContentLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const AuthorizationHeaderChangeListener& headerChangeListener) const {
  return ITwinCesiumCuratedContentLoader::createLoader(
             externals,
             tilesetOptions.contentOptions,
             this->_iTwinCesiumContentID,
             this->_iTwinAccessToken,
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