#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/Exp_TilesetContentLoaderResult.h>

#include <CesiumAsync/IAssetAccessor.h>

#include <gsl/span>

#include <string>
#include <memory>

namespace Cesium3DTilesSelection {
class TilesetJsonLoader : public TilesetContentLoader {
public:
  TilesetJsonLoader(
      const TilesetExternals& externals,
      const std::string& baseUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

  static TilesetContentLoaderResult createLoader(
      const TilesetExternals& externals,
      const std::string& baseUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const gsl::span<const std::byte>& tilesetJsonBinary);

private:
  CesiumAsync::Future<TileContentKind> doLoadTileContent(
      Tile& tile,
      const TilesetContentOptions& contentOptions) override;

  void doProcessLoadedContent(Tile& tile) override;

  void doUpdateTileContent(Tile& tile) override;

  bool doUnloadTileContent(Tile& tile) override;

  std::string _baseUrl;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _requestHeaders;
  std::vector<std::unique_ptr<TilesetContentLoader>> _children;
};
} // namespace Cesium3DTilesSelection
