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

RasterMappedTo3DTile::RasterMappedTo3DTile(
    const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile,
    const CesiumGeometry::Rectangle& textureCoordinateRectangle)
    : _pLoadingTile(pRasterTile),
      _pReadyTile(nullptr),
      _textureCoordinateID(0),
      _textureCoordinateRectangle(textureCoordinateRectangle),
      _translation(0.0, 0.0),
      _scale(1.0, 1.0),
      _state(AttachmentState::Unattached),
      _originalFailed(false) {}

RasterMappedTo3DTile::MoreDetailAvailable
RasterMappedTo3DTile::update(Tile& tile) {
  if (this->getState() == AttachmentState::Attached) {
    return !this->_originalFailed && this->_pReadyTile &&
                   this->_pReadyTile->getOverlay()
                       .getTileProvider()
                       ->hasMoreDetailsAvailable(this->_pReadyTile->getID())
               ? MoreDetailAvailable::Yes
               : MoreDetailAvailable::No;
  }

  // If the loading tile has failed, try its parent.
  while (this->_pLoadingTile && this->_pLoadingTile->getState() ==
                                    RasterOverlayTile::LoadState::Failed) {
    // Note when our original tile fails to load so that we don't report more
    // data available. This means - by design - we won't refine past a failed
    // tile.
    std::optional<TileID> parentTileId =
        TileIdUtilities::getParentTileID(this->_pLoadingTile->getID());

    // determining a parent's imagery rectangle only works for quadtrees
    // currently
    const CesiumGeometry::QuadtreeTileID* quadtreeTileId =
        parentTileId
            ? std::get_if<CesiumGeometry::QuadtreeTileID>(&*parentTileId)
            : nullptr;
    Cesium3DTiles::QuadtreeRasterOverlayTileProvider* quadtreeProvider =
        dynamic_cast<Cesium3DTiles::QuadtreeRasterOverlayTileProvider*>(
            this->_pLoadingTile->getOverlay().getTileProvider());
    std::optional<CesiumGeometry::Rectangle> parentImageryRectangle =
        (quadtreeTileId && quadtreeProvider)
            ? std::make_optional(
                  quadtreeProvider->getTilingScheme().tileToRectangle(
                      *quadtreeTileId))
            : std::nullopt;

    this->_originalFailed = true;
    this->_pLoadingTile =
        parentTileId && parentImageryRectangle
            ? this->_pLoadingTile->getOverlay().getTileProvider()->getTile(
                  *parentTileId,
                  *parentImageryRectangle)
            : nullptr;
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
    CesiumUtility::IntrusivePointer<RasterOverlayTile> pCandidate =
        this->_pLoadingTile;
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

void RasterMappedTo3DTile::detachFromTile(Tile& tile) noexcept {
  if (this->getState() == AttachmentState::Unattached) {
    return;
  }

  if (!this->_pReadyTile) {
    return;
  }

  TilesetExternals& externals = tile.getTileset()->getExternals();
  externals.pPrepareRendererResources->detachRasterInMainThread(
      tile,
      this->getTextureCoordinateID(),
      *this->_pReadyTile,
      this->_pReadyTile->getRendererResources(),
      this->getTextureCoordinateRectangle());

  this->_state = AttachmentState::Unattached;
}

void RasterMappedTo3DTile::computeTranslationAndScale(Tile& tile) {
  if (!this->_pReadyTile) {
    return;
  }

  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Cesium3DTiles::Impl::obtainGlobeRectangle(&tile.getBoundingVolume());
  if (!pRectangle) {
    return;
  }

  RasterOverlayTileProvider& tileProvider =
      *this->_pReadyTile->getOverlay().getTileProvider();
  CesiumGeometry::Rectangle geometryRectangle =
      projectRectangleSimple(tileProvider.getProjection(), *pRectangle);
  CesiumGeometry::Rectangle imageryRectangle =
      this->_pReadyTile->getImageryRectangle();

  double terrainWidth = geometryRectangle.computeWidth();
  double terrainHeight = geometryRectangle.computeHeight();

  double scaleX = terrainWidth / imageryRectangle.computeWidth();
  double scaleY = terrainHeight / imageryRectangle.computeHeight();
  this->_translation = glm::dvec2(
      (scaleX * (geometryRectangle.minimumX - imageryRectangle.minimumX)) /
          terrainWidth,
      (scaleY * (geometryRectangle.minimumY - imageryRectangle.minimumY)) /
          terrainHeight);
  this->_scale = glm::dvec2(scaleX, scaleY);
}

} // namespace Cesium3DTiles
