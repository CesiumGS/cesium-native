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

TileLoadResult TileLoadResult::createFailedResult() {
  return TileLoadResult{
      TileUnknownContent{},
      CesiumGeometry::Axis::Y,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::string(),
      {},
      RequestData{},
      TileLoadResultState::Failed};
}

TileLoadResult TileLoadResult::createRetryLaterResult() {
  return TileLoadResult{
      TileUnknownContent{},
      CesiumGeometry::Axis::Y,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::string(),
      {},
      RequestData{},
      TileLoadResultState::RetryLater};
}

TileLoadResult TileLoadResult::createRequestResult(const RequestData& request) {
  return TileLoadResult{
      TileUnknownContent{},
      CesiumGeometry::Axis::Y,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::string(),
      {},
      request,
      TileLoadResultState::RequestRequired};
}

} // namespace Cesium3DTilesSelection
