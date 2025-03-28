#include "CesiumIonTilesetLoader.h"

#include <Cesium3DTilesSelection/CesiumIonTilesetContentLoaderFactory.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>

#include <cstdint>
#include <string>
#include <utility>

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

CesiumIonTilesetContentLoaderFactory::CesiumIonTilesetContentLoaderFactory(
    uint32_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl)
    : _ionAssetID(ionAssetID),
      _ionAccessToken(ionAccessToken),
      _ionAssetEndpointUrl(ionAssetEndpointUrl) {}

bool CesiumIonTilesetContentLoaderFactory::isValid() const {
  return this->_ionAssetID > 0;
}
} // namespace Cesium3DTilesSelection