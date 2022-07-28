#include <Cesium3DTilesSelection/TilesetContentLoader.h>

namespace Cesium3DTilesSelection {
TileLoadInput::TileLoadInput(
    const Tile& tile_,
    const TilesetContentOptions& contentOptions_,
    const CesiumAsync::AsyncSystem& asyncSystem_,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor_,
    const std::shared_ptr<spdlog::logger>& pLogger_,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders_)
    : tile{tile_},
      contentOptions{contentOptions_},
      asyncSystem{asyncSystem_},
      pAssetAccessor{pAssetAccessor_},
      pLogger{pLogger_},
      requestHeaders{requestHeaders_} {}
} // namespace Cesium3DTilesSelection
