#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/TransformIterator.h>

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
 * @ref RasterOverlayCollection::updateTileOverlays.
 */
struct TileRasterOverlayStatus {
  /**
   * @brief The index of the first entry in @ref Tile::getMappedRasterTiles,
   * if any, for which more overlay detail is available than is shown by this
   * @ref Tile.
   *
   * If this is a leaf @ref Tile, an overlay with more detail available will
   * necessitate upsampling of the leaf geometry so that the overlay can be
   * rendered at full resolution.
   */
  std::optional<size_t> firstIndexWithMoreDetailAvailable;

  /**
   * @brief The index of the first entry in @ref Tile::getMappedRasterTiles,
   * if any, for which the availability of more overlay detail is not yet known.
   */
  std::optional<size_t> firstIndexWithUnknownAvailability;

  /**
   * @brief The index of the first entry in @ref Tile::getMappedRasterTiles,
   * if any, for which texture coordinates for the overlay's projection are not
   * yet available on the @ref Tile.
   */
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

  /**
   * @brief Destroys the collection.
   */
  ~RasterOverlayCollection() noexcept;

  /**
   * @brief Gets the activated raster overlays in this collection.
   */
  const std::vector<CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::ActivatedRasterOverlay>>&
  getActivatedOverlays() const noexcept;

  /**
   * @brief Adds the given {@link CesiumRasterOverlays::RasterOverlay} to this collection.
   *
   * @param pOverlay The pointer to the overlay. This may not be `nullptr`.
   */
  void add(const CesiumUtility::IntrusivePointer<
           const CesiumRasterOverlays::RasterOverlay>& pOverlay) noexcept;

  /**
   * @brief Remove the given {@link CesiumRasterOverlays::RasterOverlay} from
   * this collection.
   */
  void remove(const CesiumUtility::IntrusivePointer<
              const CesiumRasterOverlays::RasterOverlay>& pOverlay) noexcept;

  /**
   * @brief Remove the given {@link CesiumRasterOverlays::RasterOverlay} from
   * this collection.
   */
  void remove(const CesiumUtility::IntrusivePointer<
              CesiumRasterOverlays::RasterOverlay>& pOverlay) noexcept;

  /**
   * @brief Remove the given {@link CesiumRasterOverlays::ActivatedRasterOverlay}
   * from this collection.
   */
  void
  remove(const CesiumUtility::IntrusivePointer<
         CesiumRasterOverlays::ActivatedRasterOverlay>& pActivated) noexcept;

  /**
   * @brief Adds raster overlays to a new {@link Tile}.
   *
   * The tile should be in the {@link TileLoadState::Unloaded} or
   * {@link TileLoadState::FailedTemporarily} state.
   *
   * Any existing raster overlays on the tile will be cleared.
   *
   * @param tile The tile for which to add the overlays.
   * @param tilesetOptions The {@link TilesetOptions} for the tileset to which
   * the tile belongs.
   * @returns The list of projections required by the overlays that were added
   * to the tile.
   */
  std::vector<CesiumGeospatial::Projection>
  addTileOverlays(Tile& tile, const TilesetOptions& tilesetOptions) noexcept;

  /**
   * @brief Updates the raster overlays associated with a tile.
   *
   * This method is called internally for each tile that is rendered, and gives
   * the raster overlay system a chance to replace raster overlay placeholders
   * with real tiles. Its return value is also used to determine whether or not
   * this tile should be upsampled in order to attach further raster overlay
   * detail.
   *
   * @param tile The tile for which to update the overlays.
   * @param tilesetOptions The {@link TilesetOptions} for the tileset to which
   * the tile belongs.
   * @returns Details of the raster overlays attached to this tile.
   */
  TileRasterOverlayStatus
  updateTileOverlays(Tile& tile, const TilesetOptions& tilesetOptions) noexcept;

private:
  struct GetOverlayFunctor;

public:
  /**
   * @brief A constant iterator for {@link CesiumRasterOverlays::RasterOverlay} instances.
   */
  using const_iterator = CesiumUtility::TransformIterator<
      GetOverlayFunctor,
      std::vector<CesiumUtility::IntrusivePointer<
          CesiumRasterOverlays::ActivatedRasterOverlay>>::const_iterator>;

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
  struct GetOverlayFunctor {
    CesiumUtility::IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>
    operator()(const CesiumUtility::IntrusivePointer<
               CesiumRasterOverlays::ActivatedRasterOverlay>& p) const {
      return &p->getOverlay();
    }
  };

  LoadedTileEnumerator _loadedTiles;
  TilesetExternals _externals;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  std::vector<CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::ActivatedRasterOverlay>>
      _activatedOverlays;

  CESIUM_TRACE_DECLARE_TRACK_SET(_loadingSlots, "Raster Overlay Loading Slot")
};

} // namespace Cesium3DTilesSelection
