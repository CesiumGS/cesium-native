#include "ITwinCesiumCuratedContentLoader.h"

#include "CesiumIonTilesetLoader.h"

#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeospatial/Ellipsoid.h>

#include <fmt/format.h>

#include <cstdint>
#include <string>

namespace Cesium3DTilesSelection {

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
ITwinCesiumCuratedContentLoader::createLoader(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const AuthorizationHeaderChangeListener& headerChangeListener,
    bool showCreditsOnScreen,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return CesiumIonTilesetLoader::createLoader(
      externals,
      contentOptions,
      fmt::format(
          "https://api.bentley.com/curated-content/cesium/{}/tiles",
          ionAssetID),
      ionAccessToken,
      true,
      headerChangeListener,
      showCreditsOnScreen,
      ellipsoid);
}

} // namespace Cesium3DTilesSelection
