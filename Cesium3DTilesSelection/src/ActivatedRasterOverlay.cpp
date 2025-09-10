#include "ActivatedRasterOverlay.h"

#include "EmptyRasterOverlayTileProvider.h"

#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayExternals.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>

#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

ActivatedRasterOverlay::ActivatedRasterOverlay(
    const RasterOverlayExternals& externals,
    const IntrusivePointer<RasterOverlay>& pOverlay,
    const LoadedTileEnumerator& loadedTiles,
    const Ellipsoid& ellipsoid)
    : _pOverlay(pOverlay),
      _pPlaceholderTileProvider(
          pOverlay->createPlaceholder(externals, ellipsoid)),
      _pPlaceholderTile(nullptr),
      _pTileProvider(nullptr),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0) {
  this->_pPlaceholderTile = new RasterOverlayTile(
      *this->_pPlaceholderTileProvider,
      glm::dvec2(0.0),
      Rectangle());

  CesiumAsync::Future<RasterOverlay::CreateTileProviderResult> future =
      pOverlay->createTileProvider(
          externals.asyncSystem,
          externals.pAssetAccessor,
          externals.pCreditSystem,
          externals.pPrepareRendererResources,
          externals.pLogger,
          nullptr);

  // Add a placeholder for this overlay to existing geometry tiles.
  for (Tile& tile : loadedTiles) {
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
        tile.getMappedRasterTiles().emplace_back(this->_pPlaceholderTile, -1);
      }
    }
  }

  // This continuation, by capturing thiz, keeps the instance from being
  // destroyed. But it does not keep the RasterOverlayCollection itself alive.
  IntrusivePointer<ActivatedRasterOverlay> thiz = this;
  // TODO: dangerous to do this in the constructor, because no one else can
  // possibly have a reference to it yet.
  std::move(future)
      .catchInMainThread(
          [](const std::exception& e)
              -> RasterOverlay::CreateTileProviderResult {
            return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                RasterOverlayLoadType::Unknown,
                nullptr,
                fmt::format(
                    "Error while creating tile provider: {0}",
                    e.what())});
          })
      .thenInMainThread([pOverlay, thiz, externals](
                            RasterOverlay::CreateTileProviderResult&& result) {
        IntrusivePointer<RasterOverlayTileProvider> pProvider = nullptr;
        if (result) {
          pProvider = *result;
        } else {
          // Report error creating the tile provider.
          const RasterOverlayLoadFailureDetails& failureDetails =
              result.error();
          SPDLOG_LOGGER_ERROR(externals.pLogger, failureDetails.message);
          if (pOverlay->getOptions().loadErrorCallback) {
            pOverlay->getOptions().loadErrorCallback(failureDetails);
          }

          // Create a tile provider that does not provide any tiles at all.
          pProvider = new EmptyRasterOverlayTileProvider(
              pOverlay,
              externals.asyncSystem);
        }

        thiz->_pTileProvider = pProvider;
      });
}

ActivatedRasterOverlay::~ActivatedRasterOverlay() = default;

const RasterOverlay* ActivatedRasterOverlay::getOverlay() const noexcept {
  return this->_pOverlay;
}

RasterOverlay* ActivatedRasterOverlay::getOverlay() noexcept {
  return this->_pOverlay;
}

const CesiumRasterOverlays::RasterOverlayTileProvider*
ActivatedRasterOverlay::getTileProvider() const noexcept {
  return this->_pTileProvider;
}

CesiumRasterOverlays::RasterOverlayTileProvider*
ActivatedRasterOverlay::getTileProvider() noexcept {
  return this->_pTileProvider;
}

const CesiumRasterOverlays::RasterOverlayTile*
ActivatedRasterOverlay::getPlaceholderTile() const noexcept {
  return this->_pPlaceholderTile;
}

CesiumRasterOverlays::RasterOverlayTile*
ActivatedRasterOverlay::getPlaceholderTile() noexcept {
  return this->_pPlaceholderTile;
}

IntrusivePointer<RasterOverlayTile> ActivatedRasterOverlay::getTile(
    const CesiumGeometry::Rectangle& rectangle,
    const glm::dvec2& targetScreenPixels) {
  if (this->_pTileProvider == nullptr) {
    return this->_pPlaceholderTile;
  }

  if (!rectangle.overlaps(this->_pTileProvider->getCoverageRectangle())) {
    return nullptr;
  }

  return new RasterOverlayTile(
      *this->_pTileProvider,
      targetScreenPixels,
      rectangle);
}

} // namespace Cesium3DTilesSelection