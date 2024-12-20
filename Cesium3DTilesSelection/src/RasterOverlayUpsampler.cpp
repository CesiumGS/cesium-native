#include "RasterOverlayUpsampler.h"

#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumUtility/Assert.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumRasterOverlays;

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult>
RasterOverlayUpsampler::loadTileContent(const TileLoadInput& loadInput) {
  const Tile* pParent = loadInput.tile.getParent();
  if (pParent == nullptr) {
    return loadInput.asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(loadInput.pAssetAccessor, nullptr));
  }

  const CesiumGeometry::UpsampledQuadtreeNode* pTileID =
      std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(
          &loadInput.tile.getTileID());
  if (pTileID == nullptr) {
    // this tile is not marked to be upsampled, so just fail it
    return loadInput.asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(loadInput.pAssetAccessor, nullptr));
  }

  // The tile content manager guarantees that the parent tile is already loaded
  // before upsampled tile is loaded. If that's not the case, it's a bug
  CESIUM_ASSERT(
      pParent->getState() == TileLoadState::Done &&
      "Parent must be loaded before upsampling");

  const TileContent& parentContent = pParent->getContent();
  const TileRenderContent* pParentRenderContent =
      parentContent.getRenderContent();
  if (!pParentRenderContent) {
    // parent doesn't have mesh, so it's not possible to upsample
    return loadInput.asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(loadInput.pAssetAccessor, nullptr));
  }

  size_t index = 0;
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
      index = static_cast<size_t>(it - parentProjections.begin());
      break;
    }
  }

  const CesiumGeospatial::Projection& projection = parentProjections[index];
  const CesiumGeospatial::Ellipsoid& ellipsoid =
      getProjectionEllipsoid(projection);

  const CesiumGltf::Model& parentModel = pParentRenderContent->getModel();
  return loadInput.asyncSystem.runInWorkerThread(
      [&parentModel,
       ellipsoid,
       transform = loadInput.tile.getTransform(),
       textureCoordinateIndex = index,
       tileID = *pTileID,
       pAssetAccessor = loadInput.pAssetAccessor]() mutable {
        auto model = RasterOverlayUtilities::upsampleGltfForRasterOverlays(
            parentModel,
            tileID,
            false,
            RasterOverlayUtilities::DEFAULT_TEXTURE_COORDINATE_BASE_NAME,
            static_cast<int32_t>(textureCoordinateIndex),
            ellipsoid);
        if (!model) {
          return TileLoadResult::createFailedResult(pAssetAccessor, nullptr);
        }

        // Adopt the glTF up axis from the glTF itself.
        // It came from the parent, which by this point has already been
        // populated from the Tile, if necessary.
        CesiumGeometry::Axis upAxis = CesiumGeometry::Axis::Y;

        auto upIt = model->extras.find("gltfUpAxis");
        if (upIt != model->extras.end()) {
          upAxis = CesiumGeometry::Axis(
              upIt->second.getInt64OrDefault(int64_t(CesiumGeometry::Axis::Y)));
        }

        return TileLoadResult{
            std::move(*model),
            upAxis,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            nullptr,
            nullptr,
            {},
            TileLoadResultState::Success,
            ellipsoid};
      });
}

TileChildrenResult RasterOverlayUpsampler::createTileChildren(
    [[maybe_unused]] const Tile& tile,
    [[maybe_unused]] const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return {{}, TileLoadResultState::Failed};
}
} // namespace Cesium3DTilesSelection
