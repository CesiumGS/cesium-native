#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>

namespace Cesium3DTilesSelection {
class SimpleTilesetLoader : public TilesetContentLoader {
public:
  SimpleTilesetLoader(
      std::string&& baseUrl,
      std::vector<std::pair<std::string, std::string>>&& requestHeaders,
      std::string&& version);

  CesiumAsync::Future<void> requestTileContent(
      Tile& tile,
      CesiumAsync::IAssetAccessor& assetAccessor,
      CesiumAsync::AsyncSystem& asyncSystem) override;

  TileLoadState processLoadedContentSome(Tile& tile) override;

  TileLoadState processLoadedContentTillDone(Tile& tile) override;

  bool unloadTileContent(Tile& tile) override;

private:
  std::string _baseUrl;
  std::vector<std::pair<std::string, std::string>> _requestHeaders;
  std::string _version;
};
} // namespace Cesium3DTilesSelection
