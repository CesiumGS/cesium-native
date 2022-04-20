#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/Exp_TilesetContentLoaderResult.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/Future.h>

#include <gsl/span>

#include <memory>
#include <string>

namespace Cesium3DTilesSelection {
class TilesetJsonLoader : public TilesetContentLoader {
public:
  TilesetJsonLoader(
      const TilesetExternals& externals,
      const std::string& baseUrl);

  CesiumAsync::Future<TileLoadResult> doLoadTileContent(
      Tile& tile,
      const TilesetContentOptions& contentOptions,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
      override;

  void doProcessLoadedContent(Tile& tile) override;

  static CesiumAsync::Future<TilesetContentLoaderResult> createLoader(
      const TilesetExternals& externals,
      const std::string& tilesetJsonUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

  static TilesetContentLoaderResult createLoader(
      const TilesetExternals& externals,
      const std::string& baseUrl,
      const gsl::span<const std::byte>& tilesetJsonBinary);

private:
  std::string _baseUrl;
  std::vector<std::unique_ptr<TilesetContentLoader>> _children;
};
} // namespace Cesium3DTilesSelection
