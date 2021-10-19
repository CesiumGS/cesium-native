#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/RasterOverlayCollection.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/TileContentLoadResult.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "TileUtilities.h"

#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace Cesium3DTilesSelection;
using namespace CesiumUtility;

namespace {

// Find the given overlay in the given tile.
RasterOverlayTile* findTileOverlay(Tile& tile, const RasterOverlay& overlay) {
  std::vector<RasterMappedTo3DTile>& tiles = tile.getMappedRasterTiles();
  auto parentTileIt = std::find_if(
      tiles.begin(),
      tiles.end(),
      [&overlay](RasterMappedTo3DTile& raster) noexcept {
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
    const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile,
    int32_t textureCoordinateIndex)
    : _pLoadingTile(pRasterTile),
      _pReadyTile(nullptr),
      _textureCoordinateID(textureCoordinateIndex),
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
      const TilesetExternals& externals = tile.getTileset()->getExternals();
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
        const TilesetExternals& externals = tile.getTileset()->getExternals();
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

    const TilesetExternals& externals = tile.getTileset()->getExternals();
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

  const TilesetExternals& externals = tile.getTileset()->getExternals();
  externals.pPrepareRendererResources->detachRasterInMainThread(
      tile,
      this->getTextureCoordinateID(),
      *this->_pReadyTile,
      this->_pReadyTile->getRendererResources());

  this->_state = AttachmentState::Unattached;
}

bool RasterMappedTo3DTile::loadThrottled() noexcept {
  RasterOverlayTile* pLoading = this->getLoadingTile();
  if (!pLoading) {
    return true;
  }

  RasterOverlayTileProvider* pProvider =
      pLoading->getOverlay().getTileProvider();
  if (!pProvider) {
    // This should not be possible.
    assert(pProvider);
    return false;
  }

  return pProvider->loadTileThrottled(*pLoading);
}

namespace {

IntrusivePointer<RasterOverlayTile> getPlaceholderTile(RasterOverlay& overlay) {
  // Rectangle and geometric error don't matter for a placeholder.
  return overlay.getPlaceholder()->getTile(Rectangle(), 0.0);
}

const BoundingRegion*
getRegionFromBoundingVolume(const BoundingVolume& boundingVolume) {
  const BoundingRegion* pResult = std::get_if<BoundingRegion>(&boundingVolume);
  if (!pResult) {
    const BoundingRegionWithLooseFittingHeights* pLoose =
        std::get_if<BoundingRegionWithLooseFittingHeights>(&boundingVolume);
    if (pLoose) {
      pResult = &pLoose->getBoundingRegion();
    }
  }
  return pResult;
}

std::optional<Rectangle> getPreciseRectangleFromBoundingVolume(
    const Projection& projection,
    const BoundingVolume& boundingVolume) {
  const BoundingRegion* pRegion = getRegionFromBoundingVolume(boundingVolume);
  if (!pRegion) {
    return std::nullopt;
  }

  // Currently _all_ supported projections can have a rectangle precisely
  // determined from a bounding region. This may not be true, however, for
  // projections we add in the future where X is not purely a function of
  // longitude or Y is not purely a function of latitude.
  return projectRectangleSimple(projection, pRegion->getRectangle());
}

int32_t addProjectionToList(
    std::vector<Projection>& projections,
    const Projection& projection) {
  auto it = std::find(projections.begin(), projections.end(), projection);
  if (it == projections.end()) {
    projections.emplace_back(projection);
    return int32_t(projections.size()) - 1;
  } else {
    return int32_t(it - projections.begin());
  }
}

double getTileGeometricError(const Tile& tile) {
  // Use the tile's geometric error, unless it's 0.0 or really tiny, in which
  // case use half the parent's error.
  double geometricError = tile.getGeometricError();
  if (geometricError > Math::EPSILON5) {
    return geometricError;
  }

  const Tile* pParent = tile.getParent();
  double divisor = 1.0;

  while (pParent) {
    if (!pParent->getUnconditionallyRefine()) {
      divisor *= 2.0;
      double ancestorError = pParent->getGeometricError();
      if (ancestorError > Math::EPSILON5) {
        return ancestorError / divisor;
      }
    }

    pParent = pParent->getParent();
  }

  // No sensible geometric error all the way to the root of the tile tree.
  // So just use a tiny geometric error and raster selection will be limited by
  // quadtree tile count or texture resolution size.
  return Math::EPSILON5;
}

} // namespace

/*static*/ RasterMappedTo3DTile* RasterMappedTo3DTile::mapOverlayToTile(
    RasterOverlay& overlay,
    Tile& tile,
    std::vector<Projection>& missingProjections) {
  RasterOverlayTileProvider* pProvider = overlay.getTileProvider();
  if (pProvider->isPlaceholder()) {
    // Provider not created yet, so add a placeholder tile.
    return &tile.getMappedRasterTiles().emplace_back(
        RasterMappedTo3DTile(getPlaceholderTile(overlay), -1));
  }

  const Projection& projection = pProvider->getProjection();

  // If the tile is loaded, use the precise rectangle computed from the content.
  const TileContentLoadResult* pContent = tile.getContent();
  if (pContent) {
    const std::vector<Projection>& projections =
        pContent->rasterOverlayProjections;
    const std::vector<Rectangle>& rectangles =
        pContent->rasterOverlayRectangles;
    auto it = std::find(projections.begin(), projections.end(), projection);
    if (it == projections.end()) {
      // We don't have a precise rectangle for this projection, which means the
      // tile was loaded before we knew we needed this projection. We'll need to
      // reload the tile (later).
      int32_t textureCoordinateIndex =
          int32_t(projections.size()) +
          addProjectionToList(missingProjections, projection);
      // TODO: don't create a tile if there's no overlap
      return &tile.getMappedRasterTiles().emplace_back(RasterMappedTo3DTile(
          getPlaceholderTile(overlay),
          textureCoordinateIndex));
    }

    // We have a rectangle and texture coordinates for this projection.
    int32_t index = int32_t(it - projections.begin());
    assert(index < rectangles.size());

    return &tile.getMappedRasterTiles().emplace_back(RasterMappedTo3DTile(
        pProvider->getTile(rectangles[index], getTileGeometricError(tile)),
        index));
  }

  // Maybe we can derive a precise rectangle from the bounding volume.
  int32_t textureCoordinateIndex =
      addProjectionToList(missingProjections, projection);
  std::optional<Rectangle> maybeRectangle =
      getPreciseRectangleFromBoundingVolume(
          pProvider->getProjection(),
          tile.getBoundingVolume());
  if (maybeRectangle) {
    // TODO: don't create a tile if there's no overlap
    return &tile.getMappedRasterTiles().emplace_back(RasterMappedTo3DTile(
        pProvider->getTile(*maybeRectangle, getTileGeometricError(tile)),
        textureCoordinateIndex));
  } else {
    // No precise rectangle yet, so return a placeholder for now.
    return &tile.getMappedRasterTiles().emplace_back(RasterMappedTo3DTile(
        getPlaceholderTile(overlay),
        textureCoordinateIndex));
  }
}

void RasterMappedTo3DTile::computeTranslationAndScale(const Tile& tile) {
  if (!this->_pReadyTile) {
    return;
  }

  // TODO: allow no content, as long as we have a bounding region.
  if (!tile.getContent()) {
    return;
  }

  const RasterOverlayTileProvider& tileProvider =
      *this->_pReadyTile->getOverlay().getTileProvider();

  const Projection& projection = tileProvider.getProjection();
  const std::vector<Projection>& projections =
      tile.getContent()->rasterOverlayProjections;
  const std::vector<Rectangle>& rectangles =
      tile.getContent()->rasterOverlayRectangles;

  auto projectionIt =
      std::find(projections.begin(), projections.end(), projection);
  if (projectionIt == projections.end()) {
    return;
  }

  int32_t projectionIndex = int32_t(projectionIt - projections.begin());
  if (projectionIndex >= rectangles.size()) {
    return;
  }

  const Rectangle& geometryRectangle = rectangles[projectionIndex];

  const CesiumGeometry::Rectangle imageryRectangle =
      this->_pReadyTile->getRectangle();

  const double terrainWidth = geometryRectangle.computeWidth();
  const double terrainHeight = geometryRectangle.computeHeight();

  const double scaleX = terrainWidth / imageryRectangle.computeWidth();
  const double scaleY = terrainHeight / imageryRectangle.computeHeight();
  this->_translation = glm::dvec2(
      (scaleX * (geometryRectangle.minimumX - imageryRectangle.minimumX)) /
          terrainWidth,
      (scaleY * (geometryRectangle.minimumY - imageryRectangle.minimumY)) /
          terrainHeight);
  this->_scale = glm::dvec2(scaleX, scaleY);
}

} // namespace Cesium3DTilesSelection
