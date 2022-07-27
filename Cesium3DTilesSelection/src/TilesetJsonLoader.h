#pragma once

#include "TilesetContentLoader.h"
#include "TilesetContentLoaderResult.h"

#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <memory>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
class TilesetJsonLoader : public TilesetContentLoader {
public:
  TilesetJsonLoader(const std::string& baseUrl);

  CesiumAsync::Future<TileLoadResult> loadTileContent(
      const Tile& tile,
      const TilesetContentOptions& contentOptions,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
      override;

  bool updateTileContent(Tile& tile) override;

  const std::string& getBaseUrl() const noexcept;

  void addChildLoader(std::unique_ptr<TilesetContentLoader> pLoader);

  static CesiumAsync::Future<TilesetContentLoaderResult> createLoader(
      const TilesetExternals& externals,
      const std::string& tilesetJsonUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

private:
  std::string _baseUrl;
  std::vector<std::unique_ptr<TilesetContentLoader>> _children;
};
} // namespace Cesium3DTilesSelection
