#include <Cesium3DTilesSelection/TilesetContentLoader.h>

namespace Cesium3DTilesSelection {
TileLoadInput::TileLoadInput(
    const Tile& tile_,
    const TilesetContentOptions& contentOptions_,
    const CesiumAsync::AsyncSystem& asyncSystem_,
    const std::shared_ptr<spdlog::logger>& pLogger_,
    const ResponseDataMap& responsesByUrl_)
    : tile{tile_},
      contentOptions{contentOptions_},
      asyncSystem{asyncSystem_},
      pLogger{pLogger_},
      responsesByUrl{responsesByUrl_} {}

TileLoadResult TileLoadResult::createFailedResult(
    std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest) {
  return TileLoadResult{
      TileUnknownContent{},
      CesiumGeometry::Axis::Y,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::move(pCompletedRequest),
      {},
      TileLoadResultState::Failed};
}

TileLoadResult TileLoadResult::createRetryLaterResult(
    std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest) {
  return TileLoadResult{
      TileUnknownContent{},
      CesiumGeometry::Axis::Y,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::move(pCompletedRequest),
      {},
      TileLoadResultState::RetryLater};
}
} // namespace Cesium3DTilesSelection
