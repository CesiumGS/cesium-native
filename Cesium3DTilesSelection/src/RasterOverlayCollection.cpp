#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

RasterOverlayCollection::RasterOverlayCollection(
    const LoadedTileEnumerator& loadedTiles,
    const TilesetExternals& externals,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept
    : _loadedTiles(loadedTiles),
      _externals{externals},
      _ellipsoid(ellipsoid),
      _activatedOverlays() {}

RasterOverlayCollection::~RasterOverlayCollection() noexcept {
  for (int64_t i = static_cast<int64_t>(this->_activatedOverlays.size()) - 1;
       i >= 0;
       --i) {
    this->remove(this->_activatedOverlays[static_cast<size_t>(i)]);
  }
}

void RasterOverlayCollection::setLoadedTileEnumerator(
    const LoadedTileEnumerator& loadedTiles) {
  this->_loadedTiles = loadedTiles;
}

const std::vector<CesiumUtility::IntrusivePointer<
    CesiumRasterOverlays::ActivatedRasterOverlay>>&
RasterOverlayCollection::getActivatedOverlays() const noexcept {
  return this->_activatedOverlays;
}

void RasterOverlayCollection::add(
    const IntrusivePointer<const RasterOverlay>& pOverlay) noexcept {
  this->_activatedOverlays.emplace_back(pOverlay->activate(
      RasterOverlayExternals{
          .pAssetAccessor = this->_externals.pAssetAccessor,
          .pPrepareRendererResources =
              this->_externals.pPrepareRendererResources,
          .asyncSystem = this->_externals.asyncSystem,
          .pCreditSystem = this->_externals.pCreditSystem,
          .pLogger = this->_externals.pLogger},
      this->_ellipsoid));

  CesiumRasterOverlays::RasterOverlayTile* pPlaceholderTile =
      this->_activatedOverlays.back()->getPlaceholderTile();

  // Add a placeholder for this overlay to existing geometry tiles.
  for (Tile& tile : this->_loadedTiles) {
    // The tile rectangle and geometric error don't matter for a placeholder.
    // - When a tile is transitioned from Unloaded (or FailedTemporarily) to
    // ContentLoading, raster overlay tiles will be mapped to the tile
    // automatically by TilesetContentManager, so we don't need to map the
    // raster tiles to this unloaded or unloading tile now.
    // - When a tile is already failed to load, there is no need to map the
    // raster tiles to the tile as it is not rendered any way
    TileLoadState tileState = tile.getState();
    if (tileState == TileLoadState::ContentLoading ||
        tileState == TileLoadState::ContentLoaded ||
        tileState == TileLoadState::Done) {
      // Only tiles with renderable content should have raster overlays
      // attached. In the ContentLoading state, we won't know yet whether the
      // content is renderable, so assume that it is for now and
      // `setTileContent` will clear them out if necessary.
      if (tile.getContent().isRenderContent() ||
          tileState == TileLoadState::ContentLoading) {
        tile.getMappedRasterTiles().emplace_back(pPlaceholderTile, -1);
      }
    }
  }
}

void RasterOverlayCollection::remove(
    const IntrusivePointer<const RasterOverlay>& pOverlay) noexcept {
  auto it = std::find_if(
      this->_activatedOverlays.begin(),
      this->_activatedOverlays.end(),
      [pOverlay](
          const IntrusivePointer<ActivatedRasterOverlay>& pCheck) noexcept {
        return &pCheck->getOverlay() == pOverlay;
      });
  if (it == this->_activatedOverlays.end()) {
    return;
  }

  this->remove(*it);
}

void RasterOverlayCollection::remove(
    const IntrusivePointer<RasterOverlay>& pOverlay) noexcept {
  this->remove(IntrusivePointer<const RasterOverlay>(pOverlay));
}

void RasterOverlayCollection::remove(
    const IntrusivePointer<ActivatedRasterOverlay>& pActivated) noexcept {
  // Remove all mappings of this overlay to geometry tiles.
  auto removeCondition =
      [pActivated](const RasterMappedTo3DTile& mapped) noexcept {
        return (
            (mapped.getLoadingTile() &&
             pActivated == &mapped.getLoadingTile()->getActivatedOverlay()) ||
            (mapped.getReadyTile() &&
             pActivated == &mapped.getReadyTile()->getActivatedOverlay()));
      };

  auto pPrepareRenderResources =
      this->_externals.pPrepareRendererResources.get();

  for (Tile& tile : this->_loadedTiles) {
    std::vector<RasterMappedTo3DTile>& mapped = tile.getMappedRasterTiles();

    for (RasterMappedTo3DTile& rasterTile : mapped) {
      if (removeCondition(rasterTile)) {
        rasterTile.detachFromTile(*pPrepareRenderResources, tile);
      }
    }

    auto firstToRemove =
        std::remove_if(mapped.begin(), mapped.end(), removeCondition);
    mapped.erase(firstToRemove, mapped.end());
  }

  auto it = std::find(
      this->_activatedOverlays.begin(),
      this->_activatedOverlays.end(),
      pActivated);
  if (it == this->_activatedOverlays.end()) {
    return;
  }

  this->_activatedOverlays.erase(it);
}

std::vector<CesiumGeospatial::Projection>
RasterOverlayCollection::addTileOverlays(
    Tile& tile,
    const TilesetOptions& tilesetOptions) noexcept {
  // When a tile temporarily fails to load, it may still
  // have mapped raster tiles, so clear them here
  tile.getMappedRasterTiles().clear();

  std::vector<CesiumGeospatial::Projection> projections;
  const CesiumGeospatial::Ellipsoid& ellipsoid = tilesetOptions.ellipsoid;

  for (size_t i = 0; i < this->_activatedOverlays.size(); ++i) {
    ActivatedRasterOverlay& activatedOverlay = *this->_activatedOverlays[i];

    RasterMappedTo3DTile* pMapped = RasterMappedTo3DTile::mapOverlayToTile(
        tilesetOptions.maximumScreenSpaceError,
        activatedOverlay,
        tile,
        projections,
        ellipsoid);
    if (pMapped) {
      // Try to load now, but if the mapped raster tile is a placeholder this
      // won't do anything.
      pMapped->loadThrottled();
    }
  }

  return projections;
}

TileRasterOverlayStatus RasterOverlayCollection::updateTileOverlays(
    Tile& tile,
    const TilesetOptions& tilesetOptions) noexcept {
  TileRasterOverlayStatus result{};

  std::vector<RasterMappedTo3DTile>& rasterTiles = tile.getMappedRasterTiles();

  for (size_t i = 0; i < rasterTiles.size(); ++i) {
    RasterMappedTo3DTile& mappedRasterTile = rasterTiles[i];

    RasterOverlayTile* pLoadingTile = mappedRasterTile.getLoadingTile();
    if (pLoadingTile &&
        pLoadingTile->getState() == RasterOverlayTile::LoadState::Placeholder) {
      ActivatedRasterOverlay& activated = pLoadingTile->getActivatedOverlay();

      // Try to replace this placeholder with real tiles.
      if (activated.getTileProvider() != nullptr) {
        // Remove the existing placeholder mapping
        rasterTiles.erase(
            rasterTiles.begin() +
            static_cast<std::vector<RasterMappedTo3DTile>::difference_type>(i));

        // Add a new mapping.
        std::vector<CesiumGeospatial::Projection> missingProjections;
        RasterMappedTo3DTile::mapOverlayToTile(
            tilesetOptions.maximumScreenSpaceError,
            activated,
            tile,
            missingProjections,
            tilesetOptions.ellipsoid);

        if (!missingProjections.empty()) {
          if (!result.firstIndexWithMissingProjection)
            result.firstIndexWithMissingProjection = i;
          break;
        }

        --i;
      }

      continue;
    }

    const RasterOverlayTile::MoreDetailAvailable moreDetailAvailable =
        mappedRasterTile.update(
            *this->_externals.pPrepareRendererResources,
            tile);

    if (moreDetailAvailable == RasterOverlayTile::MoreDetailAvailable::Yes &&
        !result.firstIndexWithMoreDetailAvailable) {
      result.firstIndexWithMoreDetailAvailable = i;
    } else if (
        moreDetailAvailable ==
            RasterOverlayTile::MoreDetailAvailable::Unknown &&
        !result.firstIndexWithUnknownAvailability) {
      result.firstIndexWithUnknownAvailability = i;
    }
  }

  return result;
}

RasterOverlayCollection::const_iterator
RasterOverlayCollection::begin() const noexcept {
  return const_iterator(GetOverlayFunctor{}, this->_activatedOverlays.begin());
}

RasterOverlayCollection::const_iterator
RasterOverlayCollection::end() const noexcept {
  return const_iterator(GetOverlayFunctor{}, this->_activatedOverlays.end());
}

size_t RasterOverlayCollection::size() const noexcept {
  return this->_activatedOverlays.size();
}

} // namespace Cesium3DTilesSelection
