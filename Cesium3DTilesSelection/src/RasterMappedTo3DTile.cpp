#include "Cesium3DTilesSelection/RasterMappedTo3DTile.h"
#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/RasterOverlayCollection.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "TileUtilities.h"

using namespace CesiumUtility;
using namespace Cesium3DTilesSelection;

namespace {

// Find the given overlay in the given tile.
RasterOverlayTile* findTileOverlay(Tile& tile, const RasterOverlay& overlay) {
  std::vector<RasterMappedTo3DTile>& tiles = tile.getMappedRasterTiles();
  auto parentTileIt = std::find_if(
      tiles.begin(),
      tiles.end(),
      [&overlay](RasterMappedTo3DTile& raster) {
        return raster.getReadyTile() &&
               &raster.getReadyTile()->getOverlay() == &overlay;
      });
  if (parentTileIt != tiles.end()) {
    RasterMappedTo3DTile& mapped = *parentTileIt;

    // Prefer the loading tile if there is one.
    if (mapped.getLoadingTile()) {
      return mapped.getLoadingTile();
    } else {
      return mapped.getReadyTile();
    }
  }

  return nullptr;
}

} // namespace

namespace Cesium3DTilesSelection {

RasterMappedTo3DTile::RasterMappedTo3DTile(
    const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile)
    : _pLoadingTile(pRasterTile),
      _pReadyTile(nullptr),
      _textureCoordinateID(0),
      _translation(0.0, 0.0),
      _scale(1.0, 1.0),
      _state(AttachmentState::Unattached),
      _originalFailed(false) {}

RasterOverlayTile::MoreDetailAvailable
RasterMappedTo3DTile::update(Tile& tile) {
  if (this->getState() == AttachmentState::Attached) {
    return !this->_originalFailed && this->_pReadyTile &&
                   this->_pReadyTile->isMoreDetailAvailable() !=
                       RasterOverlayTile::MoreDetailAvailable::No
               ? RasterOverlayTile::MoreDetailAvailable::Yes
               : RasterOverlayTile::MoreDetailAvailable::No;
  }

  // If the loading tile has failed, try its parent's loading tile.
  Tile* pTile = &tile;
  while (this->_pLoadingTile &&
         this->_pLoadingTile->getState() ==
             RasterOverlayTile::LoadState::Failed &&
         pTile) {
    // Note when our original tile fails to load so that we don't report more
    // data available. This means - by design - we won't refine past a failed
    // tile.
    this->_originalFailed = true;

    pTile = pTile->getParent();
    if (pTile) {
      RasterOverlayTile* pOverlayTile =
          findTileOverlay(*pTile, this->_pLoadingTile->getOverlay());
      if (pOverlayTile) {
        this->_pLoadingTile = pOverlayTile;
      }
    }
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
          this->_pReadyTile->getRendererResources());
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
    CesiumUtility::IntrusivePointer<RasterOverlayTile> pCandidate;

    pTile = tile.getParent();
    while (pTile) {
      pCandidate = findTileOverlay(*pTile, this->_pLoadingTile->getOverlay());
      if (pCandidate &&
          pCandidate->getState() >= RasterOverlayTile::LoadState::Loaded) {
        break;
      }
      pTile = pTile->getParent();
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
            this->_pReadyTile->getRendererResources());
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
        this->getTranslation(),
        this->getScale());

    this->_state = this->_pLoadingTile ? AttachmentState::TemporarilyAttached
                                       : AttachmentState::Attached;
  }

  // TODO: check more precise raster overlay tile availability, rather than just
  // max level?
  if (this->_pLoadingTile) {
    return RasterOverlayTile::MoreDetailAvailable::Unknown;
  }

  if (!this->_originalFailed && this->_pReadyTile) {
    return this->_pReadyTile->isMoreDetailAvailable();
  } else {
    return RasterOverlayTile::MoreDetailAvailable::No;
  }
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
      this->_pReadyTile->getRendererResources());

  this->_state = AttachmentState::Unattached;
}

void RasterMappedTo3DTile::computeTranslationAndScale(Tile& tile) {
  if (!this->_pReadyTile) {
    return;
  }

  std::optional<CesiumGeospatial::GlobeRectangle> maybeRectangle =
      getGlobeRectangle(tile.getBoundingVolume());
  if (!maybeRectangle) {
    return;
  }

  RasterOverlayTileProvider& tileProvider =
      *this->_pReadyTile->getOverlay().getTileProvider();
  CesiumGeometry::Rectangle geometryRectangle =
      projectRectangleSimple(tileProvider.getProjection(), *maybeRectangle);
  CesiumGeometry::Rectangle imageryRectangle =
      this->_pReadyTile->getRectangle();

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

} // namespace Cesium3DTilesSelection
