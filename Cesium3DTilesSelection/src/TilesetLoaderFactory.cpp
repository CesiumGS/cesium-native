#include "CesiumIonTilesetLoader.h"
#include "ITwinCesiumCuratedContentLoader.h"

#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetLoaderFactory.h>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TilesetContentLoaderResult<TilesetContentLoader>>
CesiumIonTilesetLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const AuthorizationHeaderChangeListener& headerChangeListener) const {
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
}; // namespace Cesium3DTilesSelection