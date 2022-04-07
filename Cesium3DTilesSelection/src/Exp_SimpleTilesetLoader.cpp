#include <Cesium3DTilesSelection/Exp_SimpleTilesetLoader.h>

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
    CesiumAsync::AsyncSystem& asyncSystem) {
  (void)(tile);
  (void)(assetAccessor);
  return asyncSystem.createResolvedFuture();
}

TileLoadState
SimpleTilesetLoader::processLoadedContentSome(Tile& tile) 
{
  (void)(tile);
  return TileLoadState::Unloaded;
}

TileLoadState
SimpleTilesetLoader::processLoadedContentTillDone(Tile& tile) {
  (void)(tile);
  return TileLoadState::Unloaded;
}

bool SimpleTilesetLoader::unloadTileContent(Tile& tile) {
  (void)(tile);
  return false;
}
} // namespace Cesium3DTilesSelection
