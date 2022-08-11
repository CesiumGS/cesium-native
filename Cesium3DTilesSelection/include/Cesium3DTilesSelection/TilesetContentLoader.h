#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/RasterOverlayDetails.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGltf/Model.h>

#include <spdlog/logger.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Cesium3DTilesSelection {
class Tile;

enum class TileLoadResultState { Success, Failed, RetryLater };

using TileContentKind = std::variant<
    TileUnknownContent,
    TileEmptyContent,
    TileExternalContent,
    CesiumGltf::Model>;

struct TileLoadInput {
  TileLoadInput(
      const Tile& tile,
      const TilesetContentOptions& contentOptions,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

  const Tile& tile;
  const TilesetContentOptions& contentOptions;
  const CesiumAsync::AsyncSystem& asyncSystem;
  const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor;
  const std::shared_ptr<spdlog::logger>& pLogger;
  const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders;
};

struct TileLoadResult {
  TileContentKind contentKind;
  std::optional<BoundingVolume> updatedBoundingVolume;
  std::optional<BoundingVolume> updatedContentBoundingVolume;
  std::optional<RasterOverlayDetails> rasterOverlayDetails;
  std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest;
  std::function<void(Tile&)> tileInitializer;
  TileLoadResultState state;
};

struct TileChildrenResult {
  std::vector<Tile> children;
  TileLoadResultState state;
};

class TilesetContentLoader {
public:
  virtual ~TilesetContentLoader() = default;

  virtual CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& input) = 0;

  virtual TileChildrenResult createTileChildren(const Tile& tile) = 0;

  virtual CesiumGeometry::Axis
  getTileUpAxis(const Tile& tile) const noexcept = 0;
};
} // namespace Cesium3DTilesSelection
