#pragma once

#include "Library.h"
#include "RasterOverlay.h"
#include "RasterOverlayTileProvider.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>

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
   * @brief An iterator for {@link RasterOverlay} instances.
   */
  typedef std::vector<std::unique_ptr<RasterOverlay>>::iterator iterator;

  /**
   * @brief A constant iterator for {@link RasterOverlay} instances.
   */
  typedef std::vector<std::unique_ptr<RasterOverlay>>::const_iterator
      const_iterator;

  /**
   * @brief Returns an iterator at the beginning of this collection.
   */
  const_iterator begin() const noexcept { return this->_overlays.begin(); }

  /**
   * @brief Returns an iterator at the end of this collection.
   */
  const_iterator end() const noexcept { return this->_overlays.end(); }

  /**
   * @brief Returns an iterator at the beginning of this collection.
   */
  iterator begin() noexcept { return this->_overlays.begin(); }

  /**
   * @brief Returns an iterator at the end of this collection.
   */
  iterator end() noexcept { return this->_overlays.end(); }

  /**
   * @brief Gets the number of overlays in the collection.
   */
  size_t size() const noexcept { return this->_overlays.size(); }

private:
  Tile::LoadedLinkedList* _pLoadedTiles;
  TilesetExternals _externals;
  std::vector<std::unique_ptr<RasterOverlay>> _overlays;
  CESIUM_TRACE_DECLARE_TRACK_SET(_loadingSlots, "Raster Overlay Loading Slot");
};

} // namespace Cesium3DTilesSelection
