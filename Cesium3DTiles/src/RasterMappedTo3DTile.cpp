#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/QuadtreeRasterOverlayTileProvider.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "TileUtilities.h"
#include <optional>
#include <variant>

namespace Cesium3DTiles {

RasterToCombine::RasterToCombine(
    const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile,
    const CesiumGeometry::Rectangle textureCoordinateRectangle)
    : _pLoadingTile(pRasterTile),
      _pReadyTile(nullptr),
      _textureCoordinateRectangle(textureCoordinateRectangle),
      _translation(0.0, 0.0),
      _scale(1.0, 1.0),
      _originalFailed(false) {}

RasterMappedTo3DTile::RasterMappedTo3DTile(
    const std::vector<RasterToCombine>& rastersToCombine)
    : _rastersToCombine(rastersToCombine),
      _combinedTile(nullptr),
      _textureCoordinateRectangle(0.0, 0.0, 1.0, 1.0),
      _textureCoordinateID(0),
      _state(AttachmentState::Unattached) {}

RasterMappedTo3DTile::MoreDetailAvailable
RasterMappedTo3DTile::update(Tile& tile) {
  if (this->getState() == AttachmentState::Attached) {
    for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
      if (!rasterToCombine._originalFailed && rasterToCombine._pReadyTile) {
        const RasterOverlayTileProvider* provider =
            rasterToCombine._pReadyTile->getOverlay().getTileProvider();
        if (provider && provider->hasMoreDetailsAvailable(
                            rasterToCombine._pReadyTile->getID())) {
          return MoreDetailAvailable::Yes;
        }
      }
    }
    return MoreDetailAvailable::No;
  }

  // we need to re-combine rasters if any have changed
  bool recombineRasters = false;
  for (RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    CesiumUtility::IntrusivePointer<RasterOverlayTile>& pLoadingTile =
        rasterToCombine._pLoadingTile;
    CesiumUtility::IntrusivePointer<RasterOverlayTile>& pReadyTile =
        rasterToCombine._pReadyTile;
    bool& originalFailed = rasterToCombine._originalFailed;

    // If the loading tile has failed, try its parent.
    while (pLoadingTile &&
           pLoadingTile->getState() == RasterOverlayTile::LoadState::Failed) {
      // Note when our original tile fails to load so that we don't report more
      // data available. This means - by design - we won't refine past a failed
      // tile.
      std::optional<TileID> parentTileId =
          TileIdUtilities::getParentTileID(pLoadingTile->getID());

      // determining a parent's imagery rectangle only works for quadtrees
      // currently
      const CesiumGeometry::QuadtreeTileID* quadtreeTileId =
          parentTileId
              ? std::get_if<CesiumGeometry::QuadtreeTileID>(&*parentTileId)
              : nullptr;
      Cesium3DTiles::QuadtreeRasterOverlayTileProvider* quadtreeProvider =
          dynamic_cast<Cesium3DTiles::QuadtreeRasterOverlayTileProvider*>(
              pLoadingTile->getOverlay().getTileProvider());
      std::optional<CesiumGeometry::Rectangle> parentImageryRectangle =
          (quadtreeTileId && quadtreeProvider)
              ? std::make_optional(
                    quadtreeProvider->getTilingScheme().tileToRectangle(
                        *quadtreeTileId))
              : std::nullopt;

      originalFailed = true;
      pLoadingTile =
          parentTileId && parentImageryRectangle
              ? pLoadingTile->getOverlay().getTileProvider()->getTile(
                    *parentTileId,
                    *parentImageryRectangle)
              : nullptr;
    }

    // If the loading tile is now ready, make it the ready tile.
    if (pLoadingTile &&
        pLoadingTile->getState() >= RasterOverlayTile::LoadState::Loaded) {

      recombineRasters = true;
      /*
      // Unattach the old tile
      if (pReadyTile && this->getState() != AttachmentState::Unattached) {
        TilesetExternals& externals = tile.getTileset()->getExternals();
        externals.pPrepareRendererResources->detachRasterInMainThread(
            tile,
            this->getTextureCoordinateID(),
            *pReadyTile,
            pReadyTile->getRendererResources(),
            this->getTextureCoordinateRectangle());
        this->_state = AttachmentState::Unattached;
      }
      */

      // Mark the loading tile ready.
      pReadyTile = pLoadingTile;
      pLoadingTile = nullptr;

      // Compute the translation and scale for the new tile.
      this->computeTranslationAndScale(rasterToCombine, tile);
    }

    // Find the closest ready ancestor tile.
    if (pLoadingTile) {
      CesiumUtility::IntrusivePointer<RasterOverlayTile> pCandidate =
          pLoadingTile;
      while (pCandidate) {
        std::optional<TileID> parentTileId =
            TileIdUtilities::getParentTileID(pCandidate->getID());
        if (!parentTileId) {
          pCandidate = nullptr;
          break;
        }

        pCandidate =
            pCandidate->getOverlay().getTileProvider()->getTileWithoutCreating(
                *parentTileId);

        if (pCandidate &&
            pCandidate->getState() >= RasterOverlayTile::LoadState::Loaded) {
          break;
        }
      }

      if (pCandidate &&
          pCandidate->getState() >= RasterOverlayTile::LoadState::Loaded &&
          pReadyTile != pCandidate) {

        pReadyTile = pCandidate;

        recombineRasters = true;

        // Compute the translation and scale for the new tile.
        this->computeTranslationAndScale(rasterToCombine, tile);
      }
    }

    /*
        // Attach the ready tile if it's not already attached.
        if (pReadyTile &&
            this->getState() ==
       RasterMappedTo3DTile::AttachmentState::Unattached) {
          pReadyTile->loadInMainThread();
          recombineRasters = true;
          /*
          TilesetExternals& externals = tile.getTileset()->getExternals();
          externals.pPrepareRendererResources->attachRasterInMainThread(
              tile,
              this->getTextureCoordinateID(),
              *pReadyTile,
              pReadyTile->getRendererResources(),
              this->getTextureCoordinateRectangle(),
              this->getTranslation(),
              this->getScale());
          * /
          // TODO: change attachment states to make more sense after the current
       changes? this->_state = pLoadingTile ?
       AttachmentState::TemporarilyAttached : AttachmentState::Attached;
        }*/
  }

  const CesiumGeospatial::GlobeRectangle* pGeometryRectangle =
      Cesium3DTiles::Impl::obtainGlobeRectangle(&tile.getBoundingVolume());

  // TODO: show parents in meantime while rasters are loading/blitting
  // re-blit the rasters together
  if (this->allReady() && !this->anyLoading() && pGeometryRectangle &&
      this->_state == AttachmentState::Unattached) {
    for (RasterToCombine& rasterToCombine : this->_rastersToCombine) {
      this->computeTranslationAndScale(rasterToCombine, tile);
    }
    // is it safe to just assume all the rasters come from the same overlay?
    RasterOverlay& overlay =
        this->_rastersToCombine[0]._pReadyTile->getOverlay();
    CesiumGeometry::Rectangle geometryRectangle = projectRectangleSimple(
        overlay.getTileProvider()->getProjection(),
        *pGeometryRectangle);
    if (!this->_combinedTile) {
      this->_combinedTile = std::make_shared<RasterOverlayTile>(
          overlay,
          tile.getTileID(),
          geometryRectangle);
      // remove?
      // this->_combinedTile->addReference();
    }

    /* THINK MORE ABOUT HOW TO KNOW WHEN TO DO THIS
        // Unattach the old tile
        if (this->_combinedTile && this->getState() !=
       AttachmentState::Unattached) { TilesetExternals& externals =
       tile.getTileset()->getExternals();
          externals.pPrepareRendererResources->detachRasterInMainThread(
              tile,
              this->getTextureCoordinateID(),
              *this->_combinedTile,
              this->_combinedTile->getRendererResources(),
              this->getTextureCoordinateRectangle());
          this->_state = AttachmentState::Unattached;
        }
    */

    // don't let any other texture attach while we are attaching
    this->_state = AttachmentState::TemporarilyAttached;

    for (RasterToCombine& rasterToCombine : this->_rastersToCombine) {
      rasterToCombine._pReadyTile->addReference();
    }

    // is this safe?
    // how does the next update() call know not to reblit?
    // fix race condition on _combinedTile (use
    // RasterOverlayTileProvider::LoadResult)
    tile.getTileset()
        ->getAsyncSystem()
        .runInWorkerThread([this,
                            &externals = tile.getTileset()->getExternals(),
                            combinedTile = this->_combinedTile]() {
          std::optional<CesiumGltf::ImageCesium> image = this->blitRasters();
          if (image) {
            combinedTile->_state = RasterOverlayTile::LoadState::Loaded;
            combinedTile->_image = std::move(*image);
            // this->_combinedTile->_credits = ...
            combinedTile->_pRendererResources =
                externals.pPrepareRendererResources->prepareRasterInLoadThread(
                    *image);
          } else {
            combinedTile->_state = RasterOverlayTile::LoadState::Failed;
            combinedTile->_pRendererResources = nullptr;
          }

          return std::move(combinedTile);
        })
        .thenInMainThread(
            [this, &tile, &externals = tile.getTileset()->getExternals()](
                std::shared_ptr<RasterOverlayTile>&& combinedTile) {
              // Unattach the old tile
              /*
              if (combinedTile && this->getState() !=
              AttachmentState::Unattached) {
                externals.pPrepareRendererResources->detachRasterInMainThread(
                    tile,
                    this->getTextureCoordinateID(),
                    *combinedTile,
                    combinedTile->getRendererResources(),
                    this->getTextureCoordinateRectangle());
                this->_state = AttachmentState::Unattached;
              }

              if (combinedTile &&
                  this->getState() ==
                      RasterMappedTo3DTile::AttachmentState::Unattached) {*/
              combinedTile->loadInMainThread();

              externals.pPrepareRendererResources->attachRasterInMainThread(
                  tile,
                  this->getTextureCoordinateID(),
                  *combinedTile,
                  combinedTile->getRendererResources(),
                  this->getTextureCoordinateRectangle(),
                  glm::dvec2(0.0, 0.0),
                  glm::dvec2(1.0, 1.0));

              this->_state = this->anyLoading()
                                 ? AttachmentState::TemporarilyAttached
                                 : AttachmentState::Attached;

              for (RasterToCombine& rasterToCombine : this->_rastersToCombine) {
                rasterToCombine._pReadyTile->releaseReference();
              }
              //}
            })
        .catchInMainThread([this, &tile](const std::exception& /*e*/) {
          this->_combinedTile->_state = RasterOverlayTile::LoadState::Failed;
          this->_combinedTile->_pRendererResources = nullptr;
        });
  }

  if (this->anyLoading()) {
    return MoreDetailAvailable::Unknown;
  }

  for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    if (!rasterToCombine._originalFailed && rasterToCombine._pReadyTile) {
      const RasterOverlayTileProvider* provider =
          rasterToCombine._pReadyTile->getOverlay().getTileProvider();
      if (provider && provider->hasMoreDetailsAvailable(
                          rasterToCombine._pReadyTile->getID())) {
        return MoreDetailAvailable::Yes;
      }
    }
  }
  return MoreDetailAvailable::No;
}

void RasterMappedTo3DTile::detachFromTile(Tile& tile) noexcept {
  if (this->getState() == AttachmentState::Unattached) {
    return;
  }

  if (!this->_combinedTile) {
    return;
  }

  TilesetExternals& externals = tile.getTileset()->getExternals();
  externals.pPrepareRendererResources->detachRasterInMainThread(
      tile,
      this->getTextureCoordinateID(),
      *this->_combinedTile,
      this->_combinedTile->getRendererResources(),
      this->getTextureCoordinateRectangle());

  this->_state = AttachmentState::Unattached;
}

/*static*/ void RasterMappedTo3DTile::computeTranslationAndScale(
    RasterToCombine& rasterToCombine,
    const Tile& tile) {
  if (!rasterToCombine._pReadyTile) {
    return;
  }

  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Cesium3DTiles::Impl::obtainGlobeRectangle(&tile.getBoundingVolume());
  if (!pRectangle) {
    return;
  }

  RasterOverlayTileProvider& tileProvider =
      *rasterToCombine._pReadyTile->getOverlay().getTileProvider();
  CesiumGeometry::Rectangle geometryRectangle =
      projectRectangleSimple(tileProvider.getProjection(), *pRectangle);
  CesiumGeometry::Rectangle imageryRectangle =
      rasterToCombine._pReadyTile->getImageryRectangle();

  double terrainWidth = geometryRectangle.computeWidth();
  double terrainHeight = geometryRectangle.computeHeight();

  double scaleX = terrainWidth / imageryRectangle.computeWidth();
  double scaleY = terrainHeight / imageryRectangle.computeHeight();
  rasterToCombine._translation = glm::dvec2(
      (scaleX * (geometryRectangle.minimumX - imageryRectangle.minimumX)) /
          terrainWidth,
      (scaleY * (geometryRectangle.minimumY - imageryRectangle.minimumY)) /
          terrainHeight);
  rasterToCombine._scale = glm::dvec2(scaleX, scaleY);
}

bool RasterMappedTo3DTile::anyLoading() const noexcept {
  for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    if (rasterToCombine._pLoadingTile) {
      return true;
    }
  }
  return false;
}

bool RasterMappedTo3DTile::allReady() const noexcept {
  for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    if (!rasterToCombine._pReadyTile) {
      return false;
    }
  }
  return !this->_rastersToCombine.empty();
}

bool RasterMappedTo3DTile::hasPlaceholder() const noexcept {
  for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    if (rasterToCombine._pLoadingTile &&
        rasterToCombine._pLoadingTile->getState() ==
            RasterOverlayTile::LoadState::Placeholder) {
      return true;
    }
  }
  return false;
}

// TODO: find a way to do this type of thing on the GPU
// Will need a well thought out interface to let cesium-native use GPU resources
// while being engine-agnostic.
// TODO: probably can simplify dramatically by ignoring cases where there is
// discrepancy between channels count or bytesPerChannel between the rasters
std::optional<CesiumGltf::ImageCesium> RasterMappedTo3DTile::blitRasters() {
  if (!this->allReady()) {
    return std::nullopt;
  }

  double pixelsWidth = 1.0;
  double pixelsHeight = 1.0;
  int32_t bytesPerChannel = 1;
  int32_t channels = 1;
  for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    const CesiumGltf::ImageCesium& rasterImage =
        rasterToCombine._pReadyTile->getImage();

    // TODO: should scale divide or multiply here?
    double rasterPixelsWidth = rasterImage.width * rasterToCombine._scale.x;
    double rasterPixelsHeight = rasterImage.height * rasterToCombine._scale.y;

    if (rasterPixelsWidth > pixelsWidth) {
      pixelsWidth = rasterPixelsWidth;
    }
    if (rasterPixelsHeight > pixelsHeight) {
      pixelsHeight = rasterPixelsHeight;
    }

    if (rasterImage.bytesPerChannel > bytesPerChannel) {
      bytesPerChannel = rasterImage.bytesPerChannel;
    }
    if (rasterImage.channels > channels) {
      channels = rasterImage.channels;
    }
  }

  CesiumGltf::ImageCesium image;
  image.bytesPerChannel = bytesPerChannel;
  image.channels = channels;
  image.width = static_cast<int32_t>(glm::ceil(pixelsWidth));
  image.height = static_cast<int32_t>(glm::ceil(pixelsHeight));
  image.pixelData.resize(
      image.width * image.height * image.channels * image.bytesPerChannel);

  for (int32_t j = 0; j < image.height; ++j) {
    double v = 1.0 - double(j) / double(image.height);

    for (int32_t i = 0; i < image.width; ++i) {
      glm::dvec2 uv(double(i) / double(image.width), v);

      for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {

        if (rasterToCombine._textureCoordinateRectangle.contains(uv)) {
          const CesiumGltf::ImageCesium& srcImage =
              rasterToCombine._pReadyTile->getImage();

          glm::dvec2 srcUv =
              //(uv - rasterToCombine._translation / rasterToCombine._scale) *
              //rasterToCombine._scale;
              uv * rasterToCombine._scale + rasterToCombine._translation;

          srcUv.y = 1.0 - srcUv.y;

          // TODO: remove?
          if (srcUv.x < 0.0 || srcUv.x > 1.0 || srcUv.y < 0.0 ||
              srcUv.y > 1.0) {
            continue;
          }

          glm::dvec2 srcPixel(
              srcUv.x * srcImage.width,
              srcUv.y * srcImage.height);

          // TODO: do a sanity check of these transformedUvs
          // TODO: check if bilerp is needed here, is there any aliasing?
          uint32_t srcPixelX = static_cast<uint32_t>(
              glm::clamp(glm::round(srcPixel.x), 0.0, double(srcImage.width)));
          uint32_t srcPixelY = static_cast<uint32_t>(
              glm::clamp(glm::round(srcPixel.y), 0.0, double(srcImage.height)));

          const std::byte* pSrcPixelValue =
              srcImage.pixelData.data() +
              srcImage.channels * srcImage.bytesPerChannel *
                  (srcImage.width * srcPixelY + srcPixelX);

          std::byte* pTargetPixel =
              image.pixelData.data() +
              channels * bytesPerChannel * (image.width * j + i);

          for (int32_t channel = 0; channel < srcImage.channels; ++channel) {
            std::memcpy(
                pTargetPixel + channel * bytesPerChannel +
                    (bytesPerChannel - srcImage.bytesPerChannel),
                pSrcPixelValue + channel * srcImage.bytesPerChannel,
                srcImage.bytesPerChannel);
          }
          /*
            uint32_t dbgCol = static_cast<uint8_t>(glm::floor(srcUv.x * 255.0))
            << 24 | static_cast<uint8_t>(glm::floor(srcUv.y * 255.0)) << 16 |
                              255;

            *(uint32_t*)pTargetPixel = dbgCol;
          */
        }
      }
    }
  }

  return image;
}

} // namespace Cesium3DTiles
