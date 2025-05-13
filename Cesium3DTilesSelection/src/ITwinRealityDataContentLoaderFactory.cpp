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
    : _realityDataId(realityDataId),
      _iTwinId(iTwinId),
      _iTwinAccessToken(iTwinAccessToken),
      _tokenRefreshCallback(std::move(tokenRefreshCallback)) {}

bool ITwinRealityDataContentLoaderFactory::isValid() const {
  return !this->_realityDataId.empty() && !this->_iTwinAccessToken.empty();
}
} // namespace Cesium3DTilesSelection