#include "RasterOverlayUpsampler.h"

#include "upsampleGltfForRasterOverlays.h"

#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/RasterOverlay.h>
#include <Cesium3DTilesSelection/RasterOverlayTileProvider.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Projection.h>

#include <cassert>
#include <variant>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult> RasterOverlayUpsampler::loadTileContent(
    Tile& tile,
    [[maybe_unused]] const TilesetContentOptions& contentOptions,
    const CesiumAsync::AsyncSystem& asyncSystem,
    [[maybe_unused]] const std::shared_ptr<CesiumAsync::IAssetAccessor>&
        pAssetAccessor,
    [[maybe_unused]] const std::shared_ptr<spdlog::logger>& pLogger,
    [[maybe_unused]] const std::vector<CesiumAsync::IAssetAccessor::THeader>&
        requestHeaders) {
  Tile* pParent = tile.getParent();
  if (pParent == nullptr) {
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileUnknownContent{},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        TileLoadResultState::Failed,
        nullptr,
        {}});
  }

  const CesiumGeometry::UpsampledQuadtreeNode* pTileID =
      std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(&tile.getTileID());
  if (pTileID == nullptr) {
    // this tile is not marked to be upsampled, so just fail it
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileUnknownContent{},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        TileLoadResultState::Failed,
        nullptr,
        {}});
  }

  // The tile content manager guarantees that the parent tile is already loaded
  // before upsampled tile is loaded. If that's not the case, it's a bug
  assert(
      pParent->getState() == TileLoadState::Done &&
      "Parent must be loaded before upsampling");

  const TileContent& parentContent = pParent->getContent();
  const TileRenderContent* pParentRenderContent =
      parentContent.getRenderContent();
  if (!pParentRenderContent || !pParentRenderContent->model) {
    // parent doesn't have mesh, so it's not possible to upsample
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileUnknownContent{},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        TileLoadResultState::Failed,
        nullptr,
        {}});
  }

  int32_t index = 0;
  const std::vector<CesiumGeospatial::Projection>& parentProjections =
      parentContent.getRasterOverlayDetails()->rasterOverlayProjections;
  for (const RasterMappedTo3DTile& mapped : pParent->getMappedRasterTiles()) {
    if (mapped.isMoreDetailAvailable()) {
      const CesiumGeospatial::Projection& projection = mapped.getReadyTile()
                                                           ->getOverlay()
                                                           .getTileProvider()
                                                           ->getProjection();
      auto it = std::find(
          parentProjections.begin(),
          parentProjections.end(),
          projection);
      index = int32_t(it - parentProjections.begin());
      break;
    }
  }

  const CesiumGltf::Model& parentModel = pParentRenderContent->model.value();
  return asyncSystem.runInWorkerThread([&parentModel,
                                        transform = tile.getTransform(),
                                        textureCoordinateIndex = index,
                                        TileID = *pTileID]() mutable {
    auto model = upsampleGltfForRasterOverlays(
        parentModel,
        TileID,
        textureCoordinateIndex);

    return TileLoadResult{
        TileRenderContent{std::move(model)},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        TileLoadResultState::Success,
        nullptr,
        {}};
  });
}

bool RasterOverlayUpsampler::updateTileContent([[maybe_unused]] Tile& tile) {
  return false;
}
} // namespace Cesium3DTilesSelection
