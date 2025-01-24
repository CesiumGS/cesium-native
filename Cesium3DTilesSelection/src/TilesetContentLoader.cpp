#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeospatial/Ellipsoid.h>

#include <spdlog/logger.h>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace Cesium3DTilesSelection {
TileLoadInput::TileLoadInput(
    const Tile& tile_,
    const TilesetContentOptions& contentOptions_,
    const CesiumAsync::AsyncSystem& asyncSystem_,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor_,
    const std::shared_ptr<spdlog::logger>& pLogger_,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders_,
    const CesiumGeospatial::Ellipsoid& ellipsoid_)
    : tile{tile_},
      contentOptions{contentOptions_},
      asyncSystem{asyncSystem_},
      pAssetAccessor{pAssetAccessor_},
      pLogger{pLogger_},
      requestHeaders{requestHeaders_},
      ellipsoid(ellipsoid_) {}

TileLoadResult TileLoadResult::createFailedResult(
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor,
    std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest) {
  return TileLoadResult{
      TileUnknownContent{},
      CesiumGeometry::Axis::Y,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::move(pAssetAccessor),
      std::move(pCompletedRequest),
      {},
      TileLoadResultState::Failed,
      CesiumGeospatial::Ellipsoid::UNIT_SPHERE};
}

TileLoadResult TileLoadResult::createRetryLaterResult(
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor,
    std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest) {
  return TileLoadResult{
      TileUnknownContent{},
      CesiumGeometry::Axis::Y,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::move(pAssetAccessor),
      std::move(pCompletedRequest),
      {},
      TileLoadResultState::RetryLater,
      CesiumGeospatial::Ellipsoid::UNIT_SPHERE};
}
} // namespace Cesium3DTilesSelection
