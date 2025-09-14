#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Tracing.h>

#include <memory>
#include <span>
#include <vector>

namespace CesiumRasterOverlays {
class ActivatedRasterOverlay;
}

namespace Cesium3DTilesSelection {

class LoadedTileEnumerator;
struct TilesetOptions;

/**
 * @brief Captures the tile overlay status as produced by
 * {@link RasterOverlayCollection::updateTileOverlays}.
 */
struct TileRasterOverlayStatus {
  /**
   * @brief The index of the first entry in {@link Tile::getMappedRasterTiles},
   * if any, for which more overlay detail is available than is shown by this
   * {@link Tile}.
   *
   * If this is a leaf {@link Tile}, an overlay with more detail available will
   * necessitate upsampling of the leaf geometry so that the overlay can be
   * rendered at full resolution.
   */
  std::optional<size_t> firstIndexWithMoreDetailAvailable;

  /**
   * @brief The index of the first entry in {@link Tile::getMappedRasterTiles},
   * if any, for which the availability of more overlay detail is not yet known.
   */
  std::optional<size_t> firstIndexWithUnknownAvailability;

  std::optional<size_t> firstIndexWithMissingProjection;
};

/**
 * @brief A collection of {@link CesiumRasterOverlays::RasterOverlay} instances that are associated
 * with a {@link Tileset}.
 *
 * The raster overlay instances may be added to the raster overlay collection
 * of a tileset that is returned with {@link Tileset::getOverlays}. When the
 * tileset is loaded, one {@link CesiumRasterOverlays::RasterOverlayTileProvider} will be created
 * for each raster overlay that had been added. The raster overlay tile provider
 * instances will be passed to the {@link CesiumRasterOverlays::RasterOverlayTile} instances that
 * they create when the tiles are updated.
 */
class CESIUM3DTILESSELECTION_API RasterOverlayCollection final {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param loadedTiles An enumerator for the loaded tiles. The raster overlay
   * collection will copy this enumerator, and the copy must remain valid for
   * the lifetime of the overlay collection or until \ref
   * setLoadedTileEnumerator is called with a new enumerator.
   * @param externals A collection of loading system to load a raster overlay
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   */
  RasterOverlayCollection(
      const LoadedTileEnumerator& loadedTiles,
      const TilesetExternals& externals,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) noexcept;

  /**
   * @brief Provides a new \ref LoadedTileEnumerator to use to update
   * loaded tiles when a raster overlay is added or removed.
   *
   * The raster overlay collection will copy this enumerator, and the copy must
   * remain valid for the lifetime of the overlay collection or until \ref
   * setLoadedTileEnumerator is called with a new enumerator.
   *
   * @param loadedTiles The new loaded tile enumerator.
   */
  void setLoadedTileEnumerator(const LoadedTileEnumerator& loadedTiles);

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
   * @brief Adds the given {@link CesiumRasterOverlays::RasterOverlay} to this collection.
   *
   * @param pOverlay The pointer to the overlay. This may not be `nullptr`.
   */
  void add(const CesiumUtility::IntrusivePointer<
           const CesiumRasterOverlays::RasterOverlay>& pOverlay);

  /**
   * @brief Remove the given {@link CesiumRasterOverlays::RasterOverlay} from this collection.
   */
  void remove(const CesiumUtility::IntrusivePointer<
              const CesiumRasterOverlays::RasterOverlay>& pOverlay) noexcept;

  /**
   * @brief Adds raster overlays to a new {@link Tile}.
   *
   * The tile should be in the {@link TileLoadState::Unloaded} or
   * {@link TileLoadState::FailedTemporarily} state.
   *
   * Any existing raster overlays on the tile will be cleared.
   *
   * @param tilesetOptions The {@link TilesetOptions} for the tileset to which
   * the tile belongs.
   * @param tile The tile for which to add the overlays.
   * @returns The list of projections required by the overlays that were added
   * to the tile.
   */
  std::vector<CesiumGeospatial::Projection>
  addTileOverlays(const TilesetOptions& tilesetOptions, Tile& tile);

  /**
   * @brief Updates the raster overlays associated with a tile.
   *
   * This method is called internally for each tile that is rendered, and gives
   * the raster overlay system a chance to replace raster overlay placeholders
   * with real tiles. Its return value is also used to determine whether or not
   * this tile should be upsampled in order to attach further raster overlay
   * detail.
   *
   * @param tilesetOptions The {@link TilesetOptions} for the tileset to which
   * the tile belongs.
   * @param tile The tile for which to update the overlays.
   * @returns Details of the raster overlays attached to this tile.
   */
  TileRasterOverlayStatus
  updateTileOverlays(const TilesetOptions& tilesetOptions, Tile& tile);

  const std::vector<CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::ActivatedRasterOverlay>>&
  getActivatedOverlays() const;

  /**
   * @brief Finds the tile provider for a given overlay.
   *
   * If the specified raster overlay is not part of this collection, this method
   * will return nullptr.
   *
   * If the overlay's real tile provider hasn't finished being
   * created yet, a placeholder will be returned. That is, its
   * {@link CesiumRasterOverlays::RasterOverlayTileProvider::isPlaceholder} method will return true.
   *
   * @param overlay The overlay for which to obtain the tile provider.
   * @return The tile provider, if any, corresponding to the raster overlay.
   */
  CesiumRasterOverlays::RasterOverlayTileProvider* findTileProviderForOverlay(
      CesiumRasterOverlays::RasterOverlay& overlay) noexcept;

  /**
   * @copydoc findTileProviderForOverlay
   */
  const CesiumRasterOverlays::RasterOverlayTileProvider*
  findTileProviderForOverlay(
      const CesiumRasterOverlays::RasterOverlay& overlay) const noexcept;

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
  CesiumRasterOverlays::RasterOverlayTileProvider*
  findPlaceholderTileProviderForOverlay(
      CesiumRasterOverlays::RasterOverlay& overlay) noexcept;

  /**
   * @copydoc findPlaceholderTileProviderForOverlay
   */
  const CesiumRasterOverlays::RasterOverlayTileProvider*
  findPlaceholderTileProviderForOverlay(
      const CesiumRasterOverlays::RasterOverlay& overlay) const noexcept;

  /**
   * @brief A constant iterator for {@link CesiumRasterOverlays::RasterOverlay} instances.
   */
  typedef std::vector<CesiumUtility::IntrusivePointer<
      const CesiumRasterOverlays::RasterOverlay>>::const_iterator
      const_iterator;

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
  CesiumRasterOverlays::ActivatedRasterOverlay*
  findActivatedForOverlay(const CesiumRasterOverlays::RasterOverlay& overlay);

  struct OverlayList;

  LoadedTileEnumerator _loadedTiles;
  TilesetExternals _externals;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumUtility::IntrusivePointer<OverlayList> _pOverlays;
  CESIUM_TRACE_DECLARE_TRACK_SET(_loadingSlots, "Raster Overlay Loading Slot")
};

} // namespace Cesium3DTilesSelection
