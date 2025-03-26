#include "IModelMeshExportContentLoader.h"

#include <Cesium3DTilesSelection/IModelMeshExportContentLoaderFactory.h>

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
} // namespace Cesium3DTilesSelection