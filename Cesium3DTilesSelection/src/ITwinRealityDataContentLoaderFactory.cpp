#include "ITwinRealityDataContentLoader.h"

#include <Cesium3DTilesSelection/ITwinRealityDataContentLoaderFactory.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>

#include <optional>
#include <string>
#include <utility>

namespace Cesium3DTilesSelection {

namespace {
constexpr const char* defaultRealityDataBaseUrl =
    "https://api.bentley.com/reality-management/reality-data/";
}

CesiumAsync::Future<TilesetContentLoaderResult<TilesetContentLoader>>
ITwinRealityDataContentLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const AuthorizationHeaderChangeListener& /*headerChangeListener*/) {
  return ITwinRealityDataContentLoader::createLoader(
             externals,
             this->_realityDataId,
             this->_iTwinId,
             this->_iTwinAccessToken,
             this->_iTwinRealityDataBaseURL,
             std::move(this->_tokenRefreshCallback),
             tilesetOptions.ellipsoid)
      .thenImmediately(
          [](TilesetContentLoaderResult<ITwinRealityDataContentLoader>&&
                 result) {
            return TilesetContentLoaderResult<TilesetContentLoader>(
                std::move(result));
          });
}

ITwinRealityDataContentLoaderFactory::ITwinRealityDataContentLoaderFactory(
    const std::string& realityDataId,
    const std::optional<std::string>& iTwinId,
    const std::string& iTwinAccessToken,
    TokenRefreshCallback&& tokenRefreshCallback)
    : ITwinRealityDataContentLoaderFactory(
          realityDataId,
          iTwinId,
          iTwinAccessToken,
          defaultRealityDataBaseUrl,
          std::move(tokenRefreshCallback)) {}

ITwinRealityDataContentLoaderFactory::ITwinRealityDataContentLoaderFactory(
    const std::string& realityDataId,
    const std::optional<std::string>& iTwinId,
    const std::string& iTwinAccessToken,
    const std::string& iTwinRealityDataBaseURL,
    TokenRefreshCallback&& tokenRefreshCallback)
    : _realityDataId(realityDataId),
      _iTwinId(iTwinId),
      _iTwinAccessToken(iTwinAccessToken),
      _iTwinRealityDataBaseURL(iTwinRealityDataBaseURL),
      _tokenRefreshCallback(std::move(tokenRefreshCallback)) {
  if (!_iTwinRealityDataBaseURL.empty() &&
      _iTwinRealityDataBaseURL.back() != '/') {
    _iTwinRealityDataBaseURL.push_back('/');
  }
}

bool ITwinRealityDataContentLoaderFactory::isValid() const {
  return !this->_realityDataId.empty() && !this->_iTwinAccessToken.empty() &&
         !this->_iTwinRealityDataBaseURL.empty();
}
} // namespace Cesium3DTilesSelection
