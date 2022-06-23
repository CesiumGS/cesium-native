#include "RasterOverlayUpsampler.h"
#include <Cesium3DTilesSelection/Tile.h>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult> RasterOverlayUpsampler::loadTileContent(
    Tile& tile,
    const TilesetContentOptions& contentOptions,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  Tile* pParent = tile.getParent();
  const UpsampledQuadtreeNode* pSubdividedParentID =
      std::get_if<UpsampledQuadtreeNode>(&this->getTileID());

  assert(pParent != nullptr);
  assert(pParent->getState() == LoadState::Done);
  assert(pSubdividedParentID != nullptr);

  TileContentLoadResult* pParentContent = pParent->getContent();
  if (!pParentContent || !pParentContent->model) {
    this->setState(LoadState::ContentLoaded);
    return;
  }

  CesiumGltf::Model& parentModel = pParentContent->model.value();

  Tileset* pTileset = this->getTileset();
  pTileset->notifyTileStartLoading(this);

  struct LoadResult {
    LoadState state;
    std::unique_ptr<TileContentLoadResult> pContent;
    void* pRendererResources;
  };

  int32_t index = 0;
  std::vector<Projection>& parentProjections =
      pParentContent->overlayDetails->rasterOverlayProjections;
  const gsl::span<Tile> children = pParent->getChildren();
  if (std::get_if<QuadtreeTileID>(&pParent->getTileID()) &&
      std::any_of(
          children.begin(),
          children.end(),
          [](const Tile& tile) noexcept {
            return std::get_if<QuadtreeTileID>(&tile.getTileID());
          })) {
    // We will only get here for quantized-mesh tiles where some (but not all)
    // tiles are present in the data and the rest need to be upsampled. And in
    // that scenario, we need to use the implicit context's projection for
    // subdivision, no matter what the status of the overlays.
    //
    // For a more typical upsampling, driven by raster overlays, all child tiles
    // will have a `UpsampledQuadtreeNode` tile ID.
    if (_pContext->implicitContext->projection.has_value()) {
      const Projection& projection = *_pContext->implicitContext->projection;
      auto it = std::find(
          parentProjections.begin(),
          parentProjections.end(),
          projection);

      if (it == parentProjections.end()) {
        const BoundingRegion* pParentRegion =
            std::get_if<BoundingRegion>(&pParent->getBoundingVolume());

        std::optional<TileContentDetailsForOverlays> overlayDetails =
            GltfContent::createRasterOverlayTextureCoordinates(
                parentModel,
                pParent->getTransform(),
                int32_t(parentProjections.size()),
                pParentRegion ? std::make_optional<GlobeRectangle>(
                                    pParentRegion->getRectangle())
                              : std::nullopt,
                {projection});
        if (overlayDetails) {
          pParentContent->overlayDetails->rasterOverlayRectangles.emplace_back(
              overlayDetails->rasterOverlayRectangles[0]);
          parentProjections.emplace_back(projection);
          index = int32_t(parentProjections.size()) - 1;
        }
      } else {
        index = int32_t(it - parentProjections.begin());
      }
    }
  } else {
    for (const RasterMappedTo3DTile& mapped : pParent->getMappedRasterTiles()) {
      if (mapped.isMoreDetailAvailable()) {
        const Projection& projection = mapped.getReadyTile()
                                           ->getOverlay()
                                           .getTileProvider()
                                           ->getProjection();
        auto it = std::find(
            parentProjections.begin(),
            parentProjections.end(),
            projection);
        index = int32_t(it - parentProjections.begin());
      }
    }
  }

  pTileset->getAsyncSystem()
      .runInWorkerThread(
          [&parentModel,
           transform = this->getTransform(),
           textureCoordinateIndex = index,
           projections = std::move(projections),
           pSubdividedParentID,
           tileBoundingVolume = this->getBoundingVolume(),
           tileContentBoundingVolume = this->getContentBoundingVolume(),
           gltfUpAxis = pTileset->getGltfUpAxis(),
           generateMissingNormalsSmooth =
               pTileset->getOptions()
                   .contentOptions.generateMissingNormalsSmooth,
           pLogger = pTileset->getExternals().pLogger,
           pPrepareRendererResources =
               pTileset->getExternals().pPrepareRendererResources]() mutable {
            std::unique_ptr<TileContentLoadResult> pContent =
                std::make_unique<TileContentLoadResult>();
            pContent->model = upsampleGltfForRasterOverlays(
                parentModel,
                *pSubdividedParentID,
                textureCoordinateIndex);

            void* pRendererResources = processNewTileContent(
                pPrepareRendererResources,
                pLogger,
                *pContent,
                generateMissingNormalsSmooth,
                gltfUpAxis,
                transform,
                tileContentBoundingVolume,
                tileBoundingVolume,
                std::move(projections));

            return LoadResult{
                LoadState::ContentLoaded,
                std::move(pContent),
                pRendererResources};
          })
      .thenInMainThread([this](LoadResult&& loadResult) noexcept {
        this->_pContent = std::move(loadResult.pContent);
        this->_pRendererResources = loadResult.pRendererResources;
        this->getTileset()->notifyTileDoneLoading(this);
        this->setState(loadResult.state);
      })
      .catchInMainThread([this](const std::exception& /*e*/) noexcept {
        this->_pContent.reset();
        this->_pRendererResources = nullptr;
        this->getTileset()->notifyTileDoneLoading(this);
        this->setState(LoadState::Failed);
      });
}

bool RasterOverlayUpsampler::updateTileContent([[maybe_unused]] Tile& tile) {
  return false;
}
} // namespace Cesium3DTilesSelection
