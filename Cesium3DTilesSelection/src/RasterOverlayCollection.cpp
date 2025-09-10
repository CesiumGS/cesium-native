#include "ActivatedRasterOverlay.h"
#include "EmptyRasterOverlayTileProvider.h"

#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <fmt/format.h>
#include <glm/ext/vector_double2.hpp>
#include <nonstd/expected.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <utility>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

namespace {

// We use these to avoid a heap allocation just to return empty vectors.
const std::vector<CesiumUtility::IntrusivePointer<RasterOverlay>>
    emptyOverlays{};
const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>
    emptyTileProviders{};

} // namespace

// We store the list of overlays and tile providers in this separate class
// so that we can separate its lifetime from the lifetime of the
// RasterOverlayCollection. We need to do this because the async operations
// that create tile providers from overlays need to have somewhere to write
// the result. And we can't extend the lifetime of the entire
// RasterOverlayCollection until the async operations complete because the
// RasterOverlayCollection has a LoadedTileEnumerator, which is owned
// externally and may become invalid before the async operations complete.
struct RasterOverlayCollection::OverlayList
    : public CesiumUtility::ReferenceCountedNonThreadSafe<OverlayList> {
  std::vector<
      CesiumUtility::IntrusivePointer<CesiumRasterOverlays::RasterOverlay>>
      overlays{};
  std::vector<CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::RasterOverlayTileProvider>>
      tileProviders{};
  std::vector<CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::RasterOverlayTileProvider>>
      placeholders{};

  std::vector<CesiumUtility::IntrusivePointer<ActivatedRasterOverlay>>
      activatedOverlays{};
};

RasterOverlayCollection::RasterOverlayCollection(
    const LoadedTileEnumerator& loadedTiles,
    const TilesetExternals& externals,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept
    : _loadedTiles(loadedTiles),
      _externals{externals},
      _ellipsoid(ellipsoid),
      _pOverlays(nullptr) {}

RasterOverlayCollection::~RasterOverlayCollection() noexcept {
  if (this->_pOverlays) {
    OverlayList& list = *this->_pOverlays;
    if (!list.overlays.empty()) {
      for (int64_t i = static_cast<int64_t>(list.overlays.size() - 1); i >= 0;
           --i) {
        this->remove(list.overlays[static_cast<size_t>(i)].get());
      }
    }
  }
}

void RasterOverlayCollection::setLoadedTileEnumerator(
    const LoadedTileEnumerator& loadedTiles) {
  this->_loadedTiles = loadedTiles;
}

void RasterOverlayCollection::add(
    const CesiumUtility::IntrusivePointer<RasterOverlay>& pOverlay) {
  // CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);

  if (!this->_pOverlays)
    this->_pOverlays = new OverlayList();

  IntrusivePointer<OverlayList> pList = this->_pOverlays;

  // IntrusivePointer<ActivatedRasterOverlay>& pActivated =
  pList->activatedOverlays.emplace_back(new ActivatedRasterOverlay(
      RasterOverlayExternals{
          .pAssetAccessor = this->_externals.pAssetAccessor,
          .pPrepareRendererResources =
              this->_externals.pPrepareRendererResources,
          .asyncSystem = this->_externals.asyncSystem,
          .pCreditSystem = this->_externals.pCreditSystem,
          .pLogger = this->_externals.pLogger},
      pOverlay,
      this->_loadedTiles,
      this->_ellipsoid));
}

void RasterOverlayCollection::remove(
    const CesiumUtility::IntrusivePointer<RasterOverlay>& pOverlay) noexcept {
  if (!this->_pOverlays)
    return;

  // Remove all mappings of this overlay to geometry tiles.
  auto removeCondition = [pOverlay](
                             const RasterMappedTo3DTile& mapped) noexcept {
    return (
        (mapped.getLoadingTile() &&
         pOverlay == &mapped.getLoadingTile()->getTileProvider().getOwner()) ||
        (mapped.getReadyTile() &&
         pOverlay == &mapped.getReadyTile()->getTileProvider().getOwner()));
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

  OverlayList& list = *this->_pOverlays;

  CESIUM_ASSERT(list.overlays.size() == list.tileProviders.size());
  CESIUM_ASSERT(list.overlays.size() == list.placeholders.size());

  auto it = std::find_if(
      list.overlays.begin(),
      list.overlays.end(),
      [pOverlay](const IntrusivePointer<RasterOverlay>& pCheck) noexcept {
        return pCheck == pOverlay;
      });
  if (it == list.overlays.end()) {
    return;
  }

  int64_t index = it - list.overlays.begin();
  list.overlays.erase(list.overlays.begin() + index);
  list.tileProviders.erase(list.tileProviders.begin() + index);
  list.placeholders.erase(list.placeholders.begin() + index);
}

std::vector<CesiumGeospatial::Projection>
RasterOverlayCollection::addTileOverlays(
    const TilesetOptions& tilesetOptions,
    Tile& tile) {
  // when tile fails temporarily, it may still have mapped raster tiles, so
  // clear it here
  tile.getMappedRasterTiles().clear();

  std::vector<CesiumGeospatial::Projection> projections;

  const CesiumGeospatial::Ellipsoid& ellipsoid = tilesetOptions.ellipsoid;

  for (size_t i = 0; i < this->_pOverlays->activatedOverlays.size(); ++i) {
    ActivatedRasterOverlay& activatedOverlay =
        *this->_pOverlays->activatedOverlays[i];

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
    const TilesetOptions& tilesetOptions,
    Tile& tile) {
  TileRasterOverlayStatus result{};

  std::vector<RasterMappedTo3DTile>& rasterTiles = tile.getMappedRasterTiles();

  for (size_t i = 0; i < rasterTiles.size(); ++i) {
    RasterMappedTo3DTile& mappedRasterTile = rasterTiles[i];

    RasterOverlayTile* pLoadingTile = mappedRasterTile.getLoadingTile();
    if (pLoadingTile &&
        pLoadingTile->getState() == RasterOverlayTile::LoadState::Placeholder) {
      ActivatedRasterOverlay* pActivated =
          this->findActivatedForOverlay(pLoadingTile->getOverlay());
      CESIUM_ASSERT(pActivated);

      // Try to replace this placeholder with real tiles.
      if (pActivated && pActivated->getTileProvider() != nullptr) {
        // Remove the existing placeholder mapping
        rasterTiles.erase(
            rasterTiles.begin() +
            static_cast<std::vector<RasterMappedTo3DTile>::difference_type>(i));
        --i;

        // Add a new mapping.
        std::vector<CesiumGeospatial::Projection> missingProjections;
        RasterMappedTo3DTile::mapOverlayToTile(
            tilesetOptions.maximumScreenSpaceError,
            *pActivated,
            tile,
            missingProjections,
            tilesetOptions.ellipsoid);

        if (!missingProjections.empty()) {
          if (!result.firstIndexWithMissingProjection)
            result.firstIndexWithMissingProjection = i;
        }
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

const std::vector<CesiumUtility::IntrusivePointer<RasterOverlay>>&
RasterOverlayCollection::getOverlays() const {
  if (!this->_pOverlays)
    return emptyOverlays;

  return this->_pOverlays->overlays;
}

/**
 * @brief Gets the tile providers in this collection. Each tile provider
 * corresponds with the overlay at the same position in the collection
 * returned by {@link getOverlays}.
 */
const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
RasterOverlayCollection::getTileProviders() const {
  if (!this->_pOverlays)
    return emptyTileProviders;

  return this->_pOverlays->tileProviders;
}

const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
RasterOverlayCollection::getPlaceholderTileProviders() const {
  if (!this->_pOverlays)
    return emptyTileProviders;

  return this->_pOverlays->placeholders;
}

RasterOverlayTileProvider* RasterOverlayCollection::findTileProviderForOverlay(
    RasterOverlay& overlay) noexcept {
  // Call the const version
  const RasterOverlayTileProvider* pResult = this->findTileProviderForOverlay(
      const_cast<const RasterOverlay&>(overlay));
  return const_cast<RasterOverlayTileProvider*>(pResult);
}

const RasterOverlayTileProvider*
RasterOverlayCollection::findTileProviderForOverlay(
    const RasterOverlay& overlay) const noexcept {
  if (!this->_pOverlays)
    return nullptr;

  const auto& overlays = this->_pOverlays->overlays;
  const auto& tileProviders = this->_pOverlays->tileProviders;

  CESIUM_ASSERT(overlays.size() == tileProviders.size());

  for (size_t i = 0; i < overlays.size() && i < tileProviders.size(); ++i) {
    if (overlays[i].get() == &overlay)
      return tileProviders[i].get();
  }

  return nullptr;
}

RasterOverlayTileProvider*
RasterOverlayCollection::findPlaceholderTileProviderForOverlay(
    RasterOverlay& overlay) noexcept {
  // Call the const version
  const RasterOverlayTileProvider* pResult =
      this->findPlaceholderTileProviderForOverlay(
          const_cast<const RasterOverlay&>(overlay));
  return const_cast<RasterOverlayTileProvider*>(pResult);
}

const RasterOverlayTileProvider*
RasterOverlayCollection::findPlaceholderTileProviderForOverlay(
    const RasterOverlay& overlay) const noexcept {
  if (!this->_pOverlays)
    return nullptr;

  const auto& overlays = this->_pOverlays->overlays;
  const auto& placeholders = this->_pOverlays->placeholders;

  CESIUM_ASSERT(overlays.size() == placeholders.size());

  for (size_t i = 0; i < overlays.size() && i < placeholders.size(); ++i) {
    if (overlays[i].get() == &overlay)
      return placeholders[i].get();
  }

  return nullptr;
}

RasterOverlayCollection::const_iterator
RasterOverlayCollection::begin() const noexcept {
  if (!this->_pOverlays)
    return emptyOverlays.begin();

  return this->_pOverlays->overlays.begin();
}

RasterOverlayCollection::const_iterator
RasterOverlayCollection::end() const noexcept {
  if (!this->_pOverlays)
    return emptyOverlays.end();

  return this->_pOverlays->overlays.end();
}

size_t RasterOverlayCollection::size() const noexcept {
  if (!this->_pOverlays)
    return 0;

  return this->_pOverlays->overlays.size();
}

ActivatedRasterOverlay*
RasterOverlayCollection::findActivatedForOverlay(const RasterOverlay& overlay) {
  auto it = std::find_if(
      this->_pOverlays->activatedOverlays.begin(),
      this->_pOverlays->activatedOverlays.end(),
      [&overlay](
          const IntrusivePointer<ActivatedRasterOverlay>& pCheck) noexcept {
        return pCheck->getOverlay() == &overlay;
      });

  return it == this->_pOverlays->activatedOverlays.end() ? nullptr : it->get();
}

} // namespace Cesium3DTilesSelection
