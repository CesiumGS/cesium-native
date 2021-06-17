#include "Cesium3DTiles/QuadtreeRasterMappedTo3DTile.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/TilesetExternals.h"

namespace Cesium3DTiles {

QuadtreeRasterMappedTo3DTile::QuadtreeRasterMappedTo3DTile(
    const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile,
    const CesiumGeometry::Rectangle& textureCoordinateRectangle)
    : RasterMappedTo3DTile(pRasterTile, textureCoordinateRectangle) {}

RasterMappedTo3DTile::MoreDetailAvailable
QuadtreeRasterMappedTo3DTile::update(Tile& tile) noexcept {
  if (this->getState() == AttachmentState::Attached) {
    return !this->_originalFailed && this->_pReadyTile &&
                   this->_pReadyTile->getOverlay()
                       .getTileProvider()
                       ->hasMoreDetailsAvailable(this->_pReadyTile->getID())
               ? MoreDetailAvailable::Yes
               : MoreDetailAvailable::No;
  }

  // If the loading tile has failed, try its parent.
  const CesiumGeometry::QuadtreeTileID* quadtreeTileID;
  while (this->_pLoadingTile && this->_pLoadingTile->getState() ==
                                    RasterOverlayTile::LoadState::Failed) {

    quadtreeTileID = std::get_if<CesiumGeometry::QuadtreeTileID>(
        &this->_pLoadingTile->getID());
    if (!quadtreeTileID || quadtreeTileID->level == 0) {
      break;
    }

    // Note when our original tile fails to load so that we don't report more
    // data available. This means - by design - we won't refine past a failed
    // tile.
    this->_originalFailed = true;

    CesiumGeometry::QuadtreeTileID parentID(
        quadtreeTileID->level - 1,
        quadtreeTileID->x >> 1,
        quadtreeTileID->y >> 1);
    this->_pLoadingTile = this->_pLoadingTile->getOverlay()
                              .getTileProvider()
                              ->getTileWithoutCreating(parentID);
  }

  // If the loading tile is now ready, make it the ready tile.
  if (this->_pLoadingTile &&
      this->_pLoadingTile->getState() >= RasterOverlayTile::LoadState::Loaded) {
    // Unattach the old tile
    if (this->_pReadyTile && this->getState() != AttachmentState::Unattached) {
      TilesetExternals& externals = tile.getTileset()->getExternals();
      externals.pPrepareRendererResources->detachRasterInMainThread(
          tile,
          this->getTextureCoordinateID(),
          *this->_pReadyTile,
          this->_pReadyTile->getRendererResources(),
          this->getTextureCoordinateRectangle());
      this->_state = AttachmentState::Unattached;
    }

    // Mark the loading tile ready.
    this->_pReadyTile = this->_pLoadingTile;
    this->_pLoadingTile = nullptr;

    // Compute the translation and scale for the new tile.
    this->computeTranslationAndScale(tile);
  }

  // Find the closest ready ancestor tile.
  if (this->_pLoadingTile) {
    RasterOverlayTileProvider& tileProvider =
        *this->_pLoadingTile->getOverlay().getTileProvider();

    CesiumUtility::IntrusivePointer<RasterOverlayTile> pCandidate;
    const CesiumGeometry::QuadtreeTileID* currentID =
        std::get_if<CesiumGeometry::QuadtreeTileID>(
            &this->_pLoadingTile->getID());
    if (currentID) {
      CesiumGeometry::QuadtreeTileID id(
          currentID->level,
          currentID->x,
          currentID->y);
      while (id.level > 0) {
        --id.level;
        id.x >>= 1;
        id.y >>= 1;

        pCandidate = tileProvider.getTileWithoutCreating(id);
        if (pCandidate &&
            pCandidate->getState() >= RasterOverlayTile::LoadState::Loaded) {
          break;
        }
      }
    }

    if (pCandidate &&
        pCandidate->getState() >= RasterOverlayTile::LoadState::Loaded &&
        this->_pReadyTile != pCandidate) {
      if (this->getState() != AttachmentState::Unattached) {
        TilesetExternals& externals = tile.getTileset()->getExternals();
        externals.pPrepareRendererResources->detachRasterInMainThread(
            tile,
            this->getTextureCoordinateID(),
            *this->_pReadyTile,
            this->_pReadyTile->getRendererResources(),
            this->getTextureCoordinateRectangle());
        this->_state = AttachmentState::Unattached;
      }

      this->_pReadyTile = pCandidate;

      // Compute the translation and scale for the new tile.
      this->computeTranslationAndScale(tile);
    }
  }

  // Attach the ready tile if it's not already attached.
  if (this->_pReadyTile &&
      this->getState() == RasterMappedTo3DTile::AttachmentState::Unattached) {
    this->_pReadyTile->loadInMainThread();

    TilesetExternals& externals = tile.getTileset()->getExternals();
    externals.pPrepareRendererResources->attachRasterInMainThread(
        tile,
        this->getTextureCoordinateID(),
        *this->_pReadyTile,
        this->_pReadyTile->getRendererResources(),
        this->getTextureCoordinateRectangle(),
        this->getTranslation(),
        this->getScale());

    this->_state = this->_pLoadingTile ? AttachmentState::TemporarilyAttached
                                       : AttachmentState::Attached;
  }

  // TODO: check more precise raster overlay tile availability, rather than just
  // max level?
  if (this->_pLoadingTile) {
    return MoreDetailAvailable::Unknown;
  }
  return !this->_originalFailed && this->_pReadyTile &&
                 this->_pReadyTile->getOverlay()
                     .getTileProvider()
                     ->hasMoreDetailsAvailable(this->_pReadyTile->getID())
             ? MoreDetailAvailable::Yes
             : MoreDetailAvailable::No;
}

} // namespace Cesium3DTiles