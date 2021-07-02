#include "Cesium3DTiles/RastersMappedTo3DTile.h"
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

RastersMappedTo3DTile::RastersMappedTo3DTile(
    RasterOverlayTileProvider& owner,
    const std::vector<RasterToCombine>& rastersToCombine)
    : _pOwner(&owner),
      _rastersToCombine(rastersToCombine),
      _pCombinedTile(nullptr),
      _pAncestorRaster(nullptr),
      _textureCoordinateID(0),
      _textureCoordinateRectangle(0.0, 0.0, 1.0, 1.0),
      _state(AttachmentState::Unattached) {}

RastersMappedTo3DTile::MoreDetailAvailable
RastersMappedTo3DTile::update(Tile& tile) {
  if (this->getState() == AttachmentState::Attached) {
    for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
      if (!rasterToCombine._originalFailed && rasterToCombine._pReadyTile) {
        if (this->_pOwner->hasMoreDetailsAvailable(
                            rasterToCombine._pReadyTile->getID())) {
          return MoreDetailAvailable::Yes;
        }
      }
    }
    return MoreDetailAvailable::No;
  }

  if (!this->_pOwner) {
    return MoreDetailAvailable::No;
  }
  
  RasterOverlay& overlay = this->_pOwner->getOwner();
  const CesiumGeospatial::GlobeRectangle* pGeometryGlobeRectangle =
      Cesium3DTiles::Impl::obtainGlobeRectangle(&tile.getBoundingVolume());
  CesiumGeometry::Rectangle geometryRectangle = projectRectangleSimple(
      this->_pOwner->getProjection(),
      *pGeometryGlobeRectangle);

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
              this->_pOwner);
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

      // Mark the loading tile ready.
      pReadyTile = pLoadingTile;
      pLoadingTile = nullptr;

      // Compute the translation and scale for the new tile.
      computeTranslationAndScale(
          geometryRectangle,
          rasterToCombine._pReadyTile->_imageryRectangle,
          rasterToCombine._translation,
          rasterToCombine._scale);
    }

    // probably don't need this part any more?
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

        pCandidate = this->_pOwner->getTileWithoutCreating(*parentTileId);

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
        computeTranslationAndScale(
            geometryRectangle,
            rasterToCombine._pReadyTile->_imageryRectangle, 
            rasterToCombine._translation,
            rasterToCombine._scale);
      }
    }
  }

  /* BUGGY BEGIN */ // possibly crashy, also doesn't seem to always work
  // All the raster tiles aren't ready yet, so find an ancestor gometry tile 
  // with an already blitted raster we can use.
  if (this->_state != AttachmentState::Attached && this->anyLoading()) {

    Tile* ancestor = tile.getParent();
    std::shared_ptr<RasterOverlayTile> pAncestorRaster;
    bool candidateFound = false;
    bool noCloserAncestorFound = false;
    while (ancestor) {

      for (const RastersMappedTo3DTile& mapped : ancestor->getMappedRasterTiles()) {
        if (this->_pAncestorRaster && mapped._pCombinedTile == this->_pAncestorRaster) {
          noCloserAncestorFound;
          break;
        }

        if (mapped._pCombinedTile && &mapped._pCombinedTile->getOverlay() == &overlay &&
            mapped._state == AttachmentState::Attached) {          
          pAncestorRaster = mapped._pCombinedTile;
          candidateFound = true; 
          break;
        }
      }
      
      if (noCloserAncestorFound || candidateFound) {
        break;
      }

      ancestor = ancestor->getParent();
    }

    if (!noCloserAncestorFound && candidateFound) {
      glm::dvec2 translation;
      glm::dvec2 scale;

      computeTranslationAndScale(
        geometryRectangle,
        pAncestorRaster->_imageryRectangle,
        translation,
        scale);

      if (this->_pAncestorRaster && this->_state != AttachmentState::Unattached) {
        tile.getTileset()->getExternals().pPrepareRendererResources
            ->detachRasterInMainThread(
                  tile,
                  this->getTextureCoordinateID(),
                  *this->_pAncestorRaster,
                  this->_pAncestorRaster->getRendererResources(),
                  this->getTextureCoordinateRectangle());

        this->_pAncestorRaster = nullptr;
        this->_state = AttachmentState::Unattached;
      }

      this->_pAncestorRaster = pAncestorRaster;

      tile.getTileset()->getExternals().pPrepareRendererResources->
          attachRasterInMainThread(
            tile,
            this->getTextureCoordinateID(),
            *this->_pAncestorRaster,
            this->_pAncestorRaster->getRendererResources(),
            this->getTextureCoordinateRectangle(),
            translation,
            scale);

      this->_state = AttachmentState::TemporarilyAttached;
    }
  }
  /* BUGGY END */

  // blit the rasters together
  if (!this->_pCombinedTile && this->allReady() && !this->anyLoading() && 
      pGeometryGlobeRectangle && this->_state != AttachmentState::Attached) {
    
    this->_pCombinedTile = std::make_shared<RasterOverlayTile>(
        overlay,
        tile.getTileID(),
        geometryRectangle);

    for (RasterToCombine& rasterToCombine : this->_rastersToCombine) {
      rasterToCombine._pReadyTile->addReference();
    }

    // is any of this safe?
    // how does the next update() call know not to reblit?
    // fix race condition on _pCombinedTile (use
    // RasterOverlayTileProvider::LoadResult)
    tile.getTileset()
        ->getAsyncSystem()
        .runInWorkerThread([&externals = tile.getTileset()->getExternals(),
                            // makes copy of shared_ptr
                            combinedTile = this->_pCombinedTile,
                            &rastersToCombine = this->_rastersToCombine]() {
          std::optional<CesiumGltf::ImageCesium> image = blitRasters(rastersToCombine);
          if (image) {
            combinedTile->_state = RasterOverlayTile::LoadState::Loaded;
            combinedTile->_image = std::move(*image);
            // combinedTile->_credits = ...
            combinedTile->_pRendererResources =
                externals.pPrepareRendererResources->prepareRasterInLoadThread(
                    *image);
          } else {
            combinedTile->_state = RasterOverlayTile::LoadState::Failed;
            combinedTile->_pRendererResources = nullptr;
          }

          // TODO: needed?
          return std::move(combinedTile);
        })
        // TODO: is there a chance "this" is deleted by this point??
        .thenInMainThread(
            [this, &tile, &externals = tile.getTileset()->getExternals()](
                std::shared_ptr<RasterOverlayTile>&& combinedTile) {
              // ?? if this is needed what should be done about the load-thread raster preparation
              // (currently no preparation actually takes place in the load thread...)
              if (!this) {
                return;
              }

              combinedTile->loadInMainThread();

              // Unattach the old tile
              if (this->_pAncestorRaster && this->getState() !=
                      AttachmentState::Unattached) {
                externals.pPrepareRendererResources->detachRasterInMainThread(
                    tile,
                    this->getTextureCoordinateID(),
                    *this->_pAncestorRaster,
                    this->_pAncestorRaster->getRendererResources(),
                    this->getTextureCoordinateRectangle());

                this->_pAncestorRaster = nullptr;
                this->_state = AttachmentState::Unattached;
              }
              
              externals.pPrepareRendererResources->attachRasterInMainThread(
                  tile,
                  this->getTextureCoordinateID(),
                  *combinedTile,
                  combinedTile->getRendererResources(),
                  this->getTextureCoordinateRectangle(),
                  glm::dvec2(0.0, 0.0),
                  glm::dvec2(1.0, 1.0));

              this->_state = AttachmentState::Attached;

              for (RasterToCombine& rasterToCombine : this->_rastersToCombine) {
                rasterToCombine._pReadyTile->releaseReference();
              }
            })
        .catchInMainThread([this, &tile](const std::exception& /*e*/) {
          this->_pCombinedTile->_state = RasterOverlayTile::LoadState::Failed;
          this->_pCombinedTile->_pRendererResources = nullptr;
        });
  }

  if (this->anyLoading()) {
    return MoreDetailAvailable::Unknown;
  }

  for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    if (!rasterToCombine._originalFailed && rasterToCombine._pReadyTile) {
      if (this->_pOwner && this->_pOwner->hasMoreDetailsAvailable(
                          rasterToCombine._pReadyTile->getID())) {
        return MoreDetailAvailable::Yes;
      }
    }
  }
  return MoreDetailAvailable::No;
}

void RastersMappedTo3DTile::detachFromTile(Tile& tile) noexcept {
  if (this->getState() == AttachmentState::Unattached) {
    return;
  }

  if (!this->_pCombinedTile) {
    return;
  }

  TilesetExternals& externals = tile.getTileset()->getExternals();
  externals.pPrepareRendererResources->detachRasterInMainThread(
      tile,
      this->getTextureCoordinateID(),
      *this->_pCombinedTile,
      this->_pCombinedTile->getRendererResources(),
      this->getTextureCoordinateRectangle());

  this->_state = AttachmentState::Unattached;
}

/*static*/ void RastersMappedTo3DTile::computeTranslationAndScale(
    const CesiumGeometry::Rectangle& geometryRectangle,
    const CesiumGeometry::Rectangle& imageryRectangle,
    glm::dvec2& translation,
    glm::dvec2& scale) {

  double terrainWidth = geometryRectangle.computeWidth();
  double terrainHeight = geometryRectangle.computeHeight();

  double scaleX = terrainWidth / imageryRectangle.computeWidth();
  double scaleY = terrainHeight / imageryRectangle.computeHeight();
  translation = glm::dvec2(
      (scaleX * (geometryRectangle.minimumX - imageryRectangle.minimumX)) /
          terrainWidth,
      (scaleY * (geometryRectangle.minimumY - imageryRectangle.minimumY)) /
          terrainHeight);
  scale = glm::dvec2(scaleX, scaleY);
}

bool RastersMappedTo3DTile::anyLoading() const noexcept {
  for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    if (rasterToCombine._pLoadingTile) {
      return true;
    }
  }
  return false;
}

bool RastersMappedTo3DTile::allReady() const noexcept {
  for (const RasterToCombine& rasterToCombine : this->_rastersToCombine) {
    if (!rasterToCombine._pReadyTile) {
      return false;
    }
  }
  return !this->_rastersToCombine.empty();
}

bool RastersMappedTo3DTile::hasPlaceholder() const noexcept {
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
/*static*/ std::optional<CesiumGltf::ImageCesium> RastersMappedTo3DTile::blitRasters(
    const std::vector<RasterToCombine>& rastersToCombine) {

  double pixelsWidth = 1.0;
  double pixelsHeight = 1.0;
  int32_t bytesPerChannel = 1;
  int32_t channels = 1;
  for (const RasterToCombine& rasterToCombine : rastersToCombine) {

    if (rasterToCombine._pReadyTile == nullptr) {
      return std::nullopt;
    }

    const CesiumGltf::ImageCesium& rasterImage =
        rasterToCombine._pReadyTile->getImage();

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

  // Texture coordinates range from South (0.0) to North (1.0).
  // But pixels in images are stored in North (0) to South (imageHeight - 1)
  // order.

  for (int32_t j = 0; j < image.height; ++j) {
    // Use the texture coordinate for the _center_ of each pixel.
    // And adjust for the flipped direction of texture coordinates and pixels.
    double v = 1.0 - ((double(j) + 0.5) / double(image.height));

    for (int32_t i = 0; i < image.width; ++i) {
      glm::dvec2 uv((double(i) + 0.5) / double(image.width), v);

      for (const RasterToCombine& rasterToCombine : rastersToCombine) {

        if (rasterToCombine._textureCoordinateRectangle.contains(uv)) {
          const CesiumGltf::ImageCesium& srcImage =
              rasterToCombine._pReadyTile->getImage();

          glm::dvec2 srcUv =
              //(uv - rasterToCombine._translation / rasterToCombine._scale) *
              // rasterToCombine._scale;
              uv * rasterToCombine._scale + rasterToCombine._translation;

          // TODO: remove?
          if (srcUv.x < 0.0 || srcUv.x > 1.0 || srcUv.y < 0.0 ||
              srcUv.y > 1.0) {
            continue;
          }

          glm::dvec2 srcPixel(
              srcUv.x * srcImage.width,
              (1.0 - srcUv.y) * srcImage.height);

          // TODO: do a sanity check of these transformedUvs
          // TODO: check if bilerp is needed here, is there any aliasing?
          uint32_t srcPixelX = static_cast<uint32_t>(
              glm::clamp(glm::floor(srcPixel.x), 0.0, double(srcImage.width)));
          uint32_t srcPixelY = static_cast<uint32_t>(
              glm::clamp(glm::floor(srcPixel.y), 0.0, double(srcImage.height)));

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
