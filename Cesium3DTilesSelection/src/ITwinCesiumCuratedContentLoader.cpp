#include "ITwinCesiumCuratedContentLoader.h"

#include "CesiumIonTilesetLoader.h"

#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/Ellipsoid.h>

#include <fmt/format.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace Cesium3DTilesSelection {

class ITwinCesiumCuratedContentEndpointResource : public EndpointResource {
public:
  virtual std::string getUrl(
      int64_t ionAssetID,
      const std::string& /*ionAccessToken*/,
      const std::string& /*ionAssetEndpointUrl*/) const override {
    return fmt::format(
        "https://api.bentley.com/curated-content/cesium/{}/tiles",
        ionAssetID);
  }

  virtual bool needsAuthHeaderOnInitialRequest() const override { return true; }
};

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
      ionAssetID,
      ionAccessToken,
      "",
      ITwinCesiumCuratedContentEndpointResource{},
      headerChangeListener,
      showCreditsOnScreen,
      ellipsoid);
}

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
ITwinCesiumCuratedContentLoader::refreshTokenIfNeeded(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const AuthorizationHeaderChangeListener& headerChangeListener,
    bool showCreditsOnScreen,
    TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return CesiumIonTilesetLoader::refreshTokenIfNeeded(
      externals,
      contentOptions,
      ionAssetID,
      ionAccessToken,
      "",
      ITwinCesiumCuratedContentEndpointResource{},
      headerChangeListener,
      showCreditsOnScreen,
      std::move(result),
      ellipsoid);
}

std::shared_ptr<CesiumIonAssetAccessor>
ITwinCesiumCuratedContentLoader::createAssetAccessor(
    CesiumIonTilesetLoader& tilesetLoader,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAggregatedAccessor) {
  return std::make_shared<CesiumIonAssetAccessor>(
      tilesetLoader,
      pAggregatedAccessor,
      std::make_shared<ITwinCesiumCuratedContentEndpointResource>());
}

} // namespace Cesium3DTilesSelection