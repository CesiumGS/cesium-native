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

namespace {
constexpr const char* defaultCuratedContentUrl =
    "https://api.bentley.com/curated-content/cesium/";
}

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
             this->_iTwinURL,
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
    : ITwinCesiumCuratedContentLoaderFactory(
          iTwinCesiumContentID,
          iTwinAccessToken,
          defaultCuratedContentUrl) {}

ITwinCesiumCuratedContentLoaderFactory::ITwinCesiumCuratedContentLoaderFactory(
    uint32_t iTwinCesiumContentID,
    const std::string& iTwinAccessToken,
    const std::string& iTwinURL)
    : _iTwinCesiumContentID(iTwinCesiumContentID),
      _iTwinAccessToken(iTwinAccessToken),
      _iTwinURL(iTwinURL) {
  if (!_iTwinURL.empty() && _iTwinURL.ends_with("{}/tiles")) {
    _iTwinURL.erase(_iTwinURL.size() - 8);
  }
  if (!_iTwinURL.empty() && _iTwinURL.back() != '/') {
    _iTwinURL.push_back('/');
  }
}

bool ITwinCesiumCuratedContentLoaderFactory::isValid() const {
  return this->_iTwinCesiumContentID > 0;
}
} // namespace Cesium3DTilesSelection
