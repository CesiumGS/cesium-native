#include "IModelMeshExportContentLoader.h"

#include <Cesium3DTilesSelection/IModelMeshExportContentLoaderFactory.h>
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
IModelMeshExportContentLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const AuthorizationHeaderChangeListener& /*headerChangeListener*/) {
  return IModelMeshExportContentLoader::createLoader(
             externals,
             this->_iModelId,
             this->_exportId,
             this->_iTwinAccessToken,
             tilesetOptions.ellipsoid)
      .thenImmediately(
          [](TilesetContentLoaderResult<IModelMeshExportContentLoader>&&
                 result) {
            return TilesetContentLoaderResult<TilesetContentLoader>(
                std::move(result));
          });
}

IModelMeshExportContentLoaderFactory::IModelMeshExportContentLoaderFactory(
    const std::string& iModelId,
    const std::optional<std::string>& exportId,
    const std::string& iTwinAccessToken)
    : _iModelId(iModelId),
      _exportId(exportId),
      _iTwinAccessToken(iTwinAccessToken) {}

bool IModelMeshExportContentLoaderFactory::isValid() const {
  return !this->_iModelId.empty() && !this->_iTwinAccessToken.empty();
}
} // namespace Cesium3DTilesSelection