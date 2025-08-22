#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>

#include <spdlog/spdlog.h>

#include <string>

namespace Cesium3DTilesSelection {

class IModelMeshExportContentLoader : public TilesetContentLoader {
public:
  IModelMeshExportContentLoader(
      std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader);

  static CesiumAsync::Future<
      TilesetContentLoaderResult<IModelMeshExportContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const std::string& iModelId,
      const std::optional<std::string>& exportId,
      const std::string& iTwinAccessToken,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  TileChildrenResult createTileChildren(
      const Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid) override;

private:
  std::unique_ptr<TilesetContentLoader> _pAggregatedLoader;
};
} // namespace Cesium3DTilesSelection
