#include "ITwinCesiumCuratedContentLoader.h"

#include "CesiumIonTilesetLoader.h"

namespace Cesium3DTilesSelection {

class ITwinCesiumCuratedContentEndpointResource : public EndpointResource {
public:
  virtual std::string getUrl(
      int64_t ionAssetID,
      const std::string& /*ionAccessToken*/,
      const std::string& ionAssetEndpointUrl) const override {
    return fmt::format(
        "{}curated-content/cesium/{}/tiles",
        ionAssetEndpointUrl,
        ionAssetID);
  }
};

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
ITwinCesiumCuratedContentLoader::createLoader(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl,
    const AuthorizationHeaderChangeListener& headerChangeListener,
    bool showCreditsOnScreen,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return CesiumIonTilesetLoader::createLoader(
      externals,
      contentOptions,
      ionAssetID,
      ionAccessToken,
      ionAssetEndpointUrl,
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
    const std::string& ionAssetEndpointUrl,
    const AuthorizationHeaderChangeListener& headerChangeListener,
    bool showCreditsOnScreen,
    TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return CesiumIonTilesetLoader::refreshTokenIfNeeded(
      externals,
      contentOptions,
      ionAssetID,
      ionAccessToken,
      ionAssetEndpointUrl,
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
      ITwinCesiumCuratedContentEndpointResource{});
}

} // namespace Cesium3DTilesSelection