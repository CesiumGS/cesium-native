#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/Exp_TilesetContentLoaderResult.h>

#include <gsl/span>

#include <string>
#include <memory>

namespace Cesium3DTilesSelection {
class TilesetJsonLoader : public TilesetContentLoader {
public:
  TilesetJsonLoader(
      const TilesetExternals& externals,
      const std::string& baseUrl);

  static TilesetContentLoaderResult createLoader(
      const TilesetExternals& externals,
      const std::string& baseUrl,
      const gsl::span<const std::byte>& tilesetJsonBinary);

private:
  CesiumAsync::Future<TileContentKind>
  doLoadTileContent(const TileContentLoadInfo& loadInfo) override;

  void doProcessLoadedContent(Tile& tile) override;

  void doUpdateTileContent(Tile& tile) override;

  bool doUnloadTileContent(Tile& tile) override;

  std::string _baseUrl;
  std::unique_ptr<Tile> _root;
  std::vector<std::unique_ptr<TilesetContentLoader>> _children;
};
} // namespace Cesium3DTilesSelection
