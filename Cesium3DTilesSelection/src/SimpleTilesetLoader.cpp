#include <Cesium3DTilesSelection/SimpleTilesetLoader.h>

namespace Cesium3DTilesSelection {
SimpleTilesetLoader::SimpleTilesetLoader(
    std::string&& baseUrl,
    std::vector<std::pair<std::string, std::string>>&& requestHeaders,
    std::string&& version)
    : _baseUrl{std::move(baseUrl)},
      _requestHeaders{std::move(requestHeaders)},
      _version{std::move(version)}
{}

CesiumAsync::Future<void> SimpleTilesetLoader::requestTileContent(
    Tile& tile,
    CesiumAsync::IAssetAccessor& assetAccessor,
    CesiumAsync::AsyncSystem& asyncSystem) override {
  (void)(tile);
  (void)(assetAccessor);
  return asyncSystem.createResolvedFuture<void>();
}

TileLoadState
SimpleTilesetLoader::processLoadedContentSome(Tile& tile) override
{
  (void)(tile);
  return TileLoadState::Unloaded;
}

TileLoadState
SimpleTilesetLoader::processLoadedContentTillDone(Tile& tile) override {
  (void)(tile);
  return TileLoadState::Unloaded;
}

bool SimpleTilesetLoader::unloadTileContent(Tile& tile) override {
  (void)(tile);
  return false;
}
} // namespace Cesium3DTilesSelection
