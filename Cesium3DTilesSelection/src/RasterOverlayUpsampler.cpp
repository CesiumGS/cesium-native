#include "RasterOverlayUpsampler.h"

#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>

#include <cassert>
#include <variant>

using namespace CesiumRasterOverlays;

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult>
RasterOverlayUpsampler::loadTileContent(const TileLoadInput& loadInput) {
  const Tile* pParent = loadInput.tile.getParent();
  if (pParent == nullptr) {
    return loadInput.asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult());
  }

  const CesiumGeometry::UpsampledQuadtreeNode* pTileID =
      std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(
          &loadInput.tile.getTileID());
  if (pTileID == nullptr) {
    // this tile is not marked to be upsampled, so just fail it
    return loadInput.asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult());
  }

  // The tile content manager guarantees that the parent tile is already loaded
  // before upsampled tile is loaded. If that's not the case, it's a bug
  assert(
      pParent->getState() == TileLoadState::Done &&
      "Parent must be loaded before upsampling");

  const TileContent& parentContent = pParent->getContent();
  const TileRenderContent* pParentRenderContent =
      parentContent.getRenderContent();
  if (!pParentRenderContent) {
    // parent doesn't have mesh, so it's not possible to upsample
    return loadInput.asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult());
  }

  int32_t index = 0;
  const std::vector<CesiumGeospatial::Projection>& parentProjections =
      pParentRenderContent->getRasterOverlayDetails().rasterOverlayProjections;
  for (const RasterMappedTo3DTile& mapped : pParent->getMappedRasterTiles()) {
    if (mapped.isMoreDetailAvailable()) {
      const CesiumGeospatial::Projection& projection =
          mapped.getReadyTile()->getTileProvider().getProjection();
      auto it = std::find(
          parentProjections.begin(),
          parentProjections.end(),
          projection);
      index = int32_t(it - parentProjections.begin());
      break;
    }
  }

  const CesiumGltf::Model& parentModel = pParentRenderContent->getModel();
  return loadInput.asyncSystem.runInWorkerThread(
      [&parentModel,
       transform = loadInput.tile.getTransform(),
       textureCoordinateIndex = index,
       TileID = *pTileID]() mutable {
        auto model = RasterOverlayUtilities::upsampleGltfForRasterOverlays(
            parentModel,
            TileID,
            false,
            RasterOverlayUtilities::DEFAULT_TEXTURE_COORDINATE_BASE_NAME,
            textureCoordinateIndex);
        if (!model) {
          return TileLoadResult::createFailedResult();
        }

        return TileLoadResult{
            std::move(*model),
            CesiumGeometry::Axis::Y,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::string(),
            {},
            CesiumAsync::RequestData{},
            TileLoadResultState::Success};
      });
}

bool RasterOverlayUpsampler::getLoadWork(
    const Tile*,
    CesiumAsync::RequestData&,
    TileLoaderCallback& outCallback) {
  outCallback = [](const TileLoadInput& loadInput,
                   TilesetContentLoader* loader) {
    return loader->loadTileContent(loadInput);
  };
  return true;
}

TileChildrenResult
RasterOverlayUpsampler::createTileChildren([[maybe_unused]] const Tile& tile) {
  return {{}, TileLoadResultState::Failed};
}
} // namespace Cesium3DTilesSelection
