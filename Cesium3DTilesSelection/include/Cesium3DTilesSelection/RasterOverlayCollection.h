#pragma once

#include "Library.h"
#include "RasterOverlay.h"
#include "RasterOverlayTileProvider.h"
#include "Tile.h"
#include "TilesetExternals.h"

#include <CesiumUtility/IntrusivePointer.h>

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
  void add(std::unique_ptr<RasterOverlay>&& pOverlay);

  /**
   * @brief Remove the given {@link RasterOverlay} from this collection.
   */
  void remove(RasterOverlay* pOverlay) noexcept;

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
  struct OverlayList {
    std::vector<CesiumUtility::IntrusivePointer<RasterOverlay>> overlays;
    std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>
        tileProviders;
    int32_t _referenceCount;

    void addReference() noexcept;
    void releaseReference() noexcept;
  };

  Tile::LoadedLinkedList* _pLoadedTiles;
  TilesetExternals _externals;
  CesiumUtility::IntrusivePointer<OverlayList> _pOverlays;
  CESIUM_TRACE_DECLARE_TRACK_SET(_loadingSlots, "Raster Overlay Loading Slot");
};

} // namespace Cesium3DTilesSelection
