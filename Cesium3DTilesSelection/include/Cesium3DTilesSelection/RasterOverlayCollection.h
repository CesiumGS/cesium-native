#pragma once

#include "Library.h"
#include "RasterOverlay.h"
#include "RasterOverlayTileProvider.h"
#include "Tile.h"
#include "TilesetExternals.h"

#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedNonThreadSafe.h>
#include <CesiumUtility/Tracing.h>

#include <gsl/span>

#include <memory>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief A collection of {@link RasterOverlay} instances that are associated
 * with a {@link Tileset}.
 *
 * The raster overlay instances may be added to the raster overlay collection
 * of a tileset that is returned with {@link Tileset::getOverlays}. When the
 * tileset is loaded, one {@link RasterOverlayTileProvider} will be created
 * for each raster overlay that had been added. The raster overlay tile provider
 * instances will be passed to the {@link RasterOverlayTile} instances that
 * they create when the tiles are updated.
 */
class CESIUM3DTILESSELECTION_API RasterOverlayCollection final {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param loadedTiles The list of loaded tiles. The collection does not own
   * this list, so the list needs to be kept alive as long as the collection's
   * lifetime
   * @param externals A collection of loading system to load a raster overlay
   */
  RasterOverlayCollection(
      Tile::LoadedLinkedList& loadedTiles,
      const TilesetExternals& externals) noexcept;

  /**
   * @brief Deleted Copy constructor.
   *
   * @param rhs The other instance.
   */
  RasterOverlayCollection(const RasterOverlayCollection& rhs) = delete;

  /**
   * @brief Move constructor.
   *
   * @param rhs The other instance.
   */
  RasterOverlayCollection(RasterOverlayCollection&& rhs) noexcept = default;

  /**
   * @brief Deleted copy assignment.
   *
   * @param rhs The other instance.
   */
  RasterOverlayCollection&
  operator=(const RasterOverlayCollection& rhs) = delete;

  /**
   * @brief Move assignment.
   *
   * @param rhs The other instance.
   */
  RasterOverlayCollection&
  operator=(RasterOverlayCollection&& rhs) noexcept = default;

  ~RasterOverlayCollection() noexcept;

  /**
   * @brief Adds the given {@link RasterOverlay} to this collection.
   *
   * @param pOverlay The pointer to the overlay. This may not be `nullptr`.
   */
  void add(const CesiumUtility::IntrusivePointer<RasterOverlay>& pOverlay);

  /**
   * @brief Remove the given {@link RasterOverlay} from this collection.
   */
  void remove(
      const CesiumUtility::IntrusivePointer<RasterOverlay>& pOverlay) noexcept;

  /**
   * @brief Gets the overlays in this collection.
   */
  const std::vector<CesiumUtility::IntrusivePointer<RasterOverlay>>&
  getOverlays() const;

  /**
   * @brief Gets the tile providers in this collection. Each tile provider
   * corresponds with the overlay at the same position in the collection
   * returned by {@link getOverlays}.
   */
  const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
  getTileProviders() const;

  /**
   * @brief Gets the placeholder tile providers in this collection. Each
   * placeholder tile provider corresponds with the overlay at the same position
   * in the collection returned by {@link getOverlays}.
   */
  const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
  getPlaceholderTileProviders() const;

  /**
   * @brief Finds the tile provider for a given overlay.
   *
   * If the specified raster overlay is not part of this collection, this method
   * will return nullptr.
   *
   * If the overlay's real tile provider hasn't finished being
   * created yet, a placeholder will be returned. That is, its
   * {@link RasterOverlayTileProvider::isPlaceholder} method will return true.
   *
   * @param overlay The overlay for which to obtain the tile provider.
   * @return The tile provider, if any, corresponding to the raster overlay.
   */
  RasterOverlayTileProvider*
  findTileProviderForOverlay(RasterOverlay& overlay) noexcept;

  /**
   * @copydoc findTileProviderForOverlay
   */
  const RasterOverlayTileProvider*
  findTileProviderForOverlay(const RasterOverlay& overlay) const noexcept;

  /**
   * @brief Finds the placeholder tile provider for a given overlay.
   *
   * If the specified raster overlay is not part of this collection, this method
   * will return nullptr.
   *
   * This method will return the placeholder tile provider even if the real one
   * has been created. This is useful to create placeholder tiles when the
   * rectangle in the overlay's projection is not yet known.
   *
   * @param overlay The overlay for which to obtain the tile provider.
   * @return The placeholder tile provider, if any, corresponding to the raster
   * overlay.
   */
  RasterOverlayTileProvider*
  findPlaceholderTileProviderForOverlay(RasterOverlay& overlay) noexcept;

  /**
   * @copydoc findPlaceholderTileProviderForOverlay
   */
  const RasterOverlayTileProvider* findPlaceholderTileProviderForOverlay(
      const RasterOverlay& overlay) const noexcept;

  /**
   * @brief A constant iterator for {@link RasterOverlay} instances.
   */
  typedef std::vector<CesiumUtility::IntrusivePointer<RasterOverlay>>::
      const_iterator const_iterator;

  /**
   * @brief Returns an iterator at the beginning of this collection.
   */
  const_iterator begin() const noexcept;

  /**
   * @brief Returns an iterator at the end of this collection.
   */
  const_iterator end() const noexcept;

  /**
   * @brief Gets the number of overlays in the collection.
   */
  size_t size() const noexcept;

private:
  // We store the list of overlays and tile providers in this separate class
  // so that we can separate its lifetime from the lifetime of the
  // RasterOverlayCollection. We need to do this because the async operations
  // that create tile providers from overlays need to have somewhere to write
  // the result. And we can't extend the lifetime of the entire
  // RasterOverlayCollection until the async operations complete because the
  // RasterOverlayCollection has a pointer to the tile LoadedLinkedList, which
  // is owned externally and may become invalid before the async operations
  // complete.
  struct OverlayList
      : public CesiumUtility::ReferenceCountedNonThreadSafe<OverlayList> {
    std::vector<CesiumUtility::IntrusivePointer<RasterOverlay>> overlays{};
    std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>
        tileProviders{};
    std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>
        placeholders{};
  };

  Tile::LoadedLinkedList* _pLoadedTiles;
  TilesetExternals _externals;
  CesiumUtility::IntrusivePointer<OverlayList> _pOverlays;
  CESIUM_TRACE_DECLARE_TRACK_SET(_loadingSlots, "Raster Overlay Loading Slot");
};

} // namespace Cesium3DTilesSelection
