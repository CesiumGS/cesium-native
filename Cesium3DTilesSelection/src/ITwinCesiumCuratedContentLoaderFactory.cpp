#include "CesiumIonTilesetLoader.h"
#include "ITwinCesiumCuratedContentLoader.h"

#include <Cesium3DTilesSelection/ITwinCesiumCuratedContentLoaderFactory.h>
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
ITwinCesiumCuratedContentLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const AuthorizationHeaderChangeListener& headerChangeListener) {
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

ITwinCesiumCuratedContentLoaderFactory::ITwinCesiumCuratedContentLoaderFactory(
    uint32_t iTwinCesiumContentID,
    const std::string& iTwinAccessToken)
    : _iTwinCesiumContentID(iTwinCesiumContentID),
      _iTwinAccessToken(iTwinAccessToken) {}

bool ITwinCesiumCuratedContentLoaderFactory::isValid() const {
  return this->_iTwinCesiumContentID > 0;
}
} // namespace Cesium3DTilesSelection