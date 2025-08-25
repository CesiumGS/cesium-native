#pragma once

#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <memory>

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief The result of applying a {@link CesiumRasterOverlays::RasterOverlayTile} to geometry.
 *
 * Instances of this class are used by a {@link Tile} in order to map
 * imagery data that is given as {@link CesiumRasterOverlays::RasterOverlayTile} instances
 * to the 2D region that is covered by the tile geometry.
 */
class RasterMappedTo3DTile final {
public:
  /**
   * @brief The states indicating whether the raster tile is attached to the
   * geometry.
   */
  enum class AttachmentState {
    /**
     * @brief This raster tile is not yet attached to the geometry at all.
     */
    Unattached = 0,

    /**
     * @brief This raster tile is attached to the geometry, but it is a
     * temporary, low-res version usable while the full-res version is loading.
     */
    TemporarilyAttached = 1,

    /**
     * @brief This raster tile is attached to the geometry.
     */
    Attached = 2
  };

  /**
   * @brief Creates a new instance.
   *
   * @param pRasterTile The {@link CesiumRasterOverlays::RasterOverlayTile} that is mapped to the
   * geometry.
   * @param textureCoordinateIndex The index of the texture coordinates to use
   * with this mapped raster overlay.
   */
  RasterMappedTo3DTile(
      const CesiumUtility::IntrusivePointer<
          CesiumRasterOverlays::RasterOverlayTile>& pRasterTile,
      int32_t textureCoordinateIndex);

  /**
   * @brief Returns a {@link CesiumRasterOverlays::RasterOverlayTile} that is currently loading.
   *
   * The caller has to check the exact state of this tile, using
   * {@link Tile::getState}.
   *
   * @return The tile that is loading, or `nullptr`.
   */
  CesiumRasterOverlays::RasterOverlayTile* getLoadingTile() noexcept {
    return this->_pLoadingTile.get();
  }

  /** @copydoc getLoadingTile */
  const CesiumRasterOverlays::RasterOverlayTile*
  getLoadingTile() const noexcept {
    return this->_pLoadingTile.get();
  }

  /**
   * @brief Returns the {@link CesiumRasterOverlays::RasterOverlayTile} that represents the imagery
   * data that is ready to render.
   *
   * This will be `nullptr` when the tile data has not yet been loaded.
   *
   * @return The tile, or `nullptr`.
   */
  CesiumRasterOverlays::RasterOverlayTile* getReadyTile() noexcept {
    return this->_pReadyTile.get();
  }

  /** @copydoc getReadyTile */
  const CesiumRasterOverlays::RasterOverlayTile* getReadyTile() const noexcept {
    return this->_pReadyTile.get();
  }

  /**
   * @brief Returns an identifier for the texture coordinates of this tile.
   *
   * The texture coordinates for this raster are found in the glTF as an
   * attribute named `_CESIUMOVERLAY_n` where `n` is this value.
   *
   * @return The texture coordinate ID.
   */
  int32_t getTextureCoordinateID() const noexcept {
    return this->_textureCoordinateID;
  }

  /**
   * @brief Sets the texture coordinate ID.
   *
   * @see getTextureCoordinateID
   *
   * @param textureCoordinateID The ID.
   */
  void setTextureCoordinateID(int32_t textureCoordinateID) noexcept {
    this->_textureCoordinateID = textureCoordinateID;
  }

  /**
   * @brief Returns the translation that converts between the geometry texture
   * coordinates and the texture coordinates that should be used to sample this
   * raster texture.
   *
   * `rasterCoordinates = geometryCoordinates * scale + translation`
   *
   * @returns The translation.
   */
  const glm::dvec2& getTranslation() const noexcept {
    return this->_translation;
  }

  /**
   * @brief Returns the scaling that converts between the geometry texture
   * coordinates and the texture coordinates that should be used to sample this
   * raster texture.
   *
   * @see getTranslation
   *
   * @returns The scaling.
   */
  const glm::dvec2& getScale() const noexcept { return this->_scale; }

  /**
   * @brief Indicates whether this overlay tile is currently attached to its
   * owning geometry tile.
   *
   * When a raster overlay tile is attached to a geometry tile,
   * {@link IPrepareRendererResources::attachRasterInMainThread} is invoked.
   * When it is detached,
   * {@link IPrepareRendererResources::detachRasterInMainThread} is invoked.
   */
  AttachmentState getState() const noexcept { return this->_state; }

  /**
   * @brief Update this tile during the update of its owner.
   *
   * This is only supposed to be called by
   * `TilesetContentManager::updateDoneState`. It will return whether there is a
   * more detailed version of the raster data available.
   *
   * @param prepareRendererResources The {@link IPrepareRendererResources} used to
   * create render resources for raster overlay
   * @param tile The owner tile.
   * @return The {@link CesiumRasterOverlays::RasterOverlayTile::MoreDetailAvailable} state.
   */
  CesiumRasterOverlays::RasterOverlayTile::MoreDetailAvailable
  update(IPrepareRendererResources& prepareRendererResources, Tile& tile);

  /**
   * @copydoc CesiumRasterOverlays::RasterOverlayTile::isMoreDetailAvailable
   */
  bool isMoreDetailAvailable() const noexcept;

  /**
   * @brief Detach the raster from the given tile.
   * @param prepareRendererResources The IPrepareRendererResources used to
   * detach raster overlay from the tile geometry
   * @param tile The owner tile.
   */
  void detachFromTile(
      IPrepareRendererResources& prepareRendererResources,
      Tile& tile) noexcept;

  /**
   * @brief Does a throttled load of the mapped {@link CesiumRasterOverlays::RasterOverlayTile}.
   *
   * @return If the mapped tile is already in the process of loading or it has
   * already finished loading, this method does nothing and returns true. If too
   * many loads are already in progress, this method does nothing and returns
   * false. Otherwise, it begins the asynchronous process to load the tile and
   * returns true.
   */
  bool loadThrottled() noexcept;

  /**
   * @brief Creates a maping between a {@link CesiumRasterOverlays::RasterOverlay} and a {@link Tile}.
   *
   * The returned mapping will be to a placeholder {@link CesiumRasterOverlays::RasterOverlayTile} if
   * the overlay's tile provider is not yet ready (i.e. it's still a
   * placeholder) or if the overlap between the tile and the raster overlay
   * cannot yet be determined because the projected rectangle of the tile is not
   * yet known.
   *
   * Returns a pointer to the created `RasterMappedTo3DTile` in the Tile's
   * {@link Tile::getMappedRasterTiles} collection. Note that this pointer may
   * become invalid as soon as another item is added to or removed from this
   * collection.
   *
   * @param maximumScreenSpaceError The maximum screen space error that is used
   * for the current tile
   * @param tileProvider The overlay tile provider to map to the tile. This may
   * be a placeholder if the tile provider is not yet ready.
   * @param placeholder The placeholder tile provider for this overlay. This is
   * always a placeholder, even if the tile provider is already ready.
   * @param tile The tile to which to map the overlay.
   * @param missingProjections The list of projections for which there are not
   * yet any texture coordiantes. On return, the given overlay's Projection may
   * be added to this collection if the Tile does not yet have texture
   * coordinates for the Projection and the Projection is not already in the
   * collection.
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   * @return A pointer the created mapping, which may be to a placeholder, or
   * nullptr if no mapping was created at all because the Tile does not overlap
   * the raster overlay.
   */
  static RasterMappedTo3DTile* mapOverlayToTile(
      double maximumScreenSpaceError,
      CesiumRasterOverlays::RasterOverlayTileProvider& tileProvider,
      CesiumRasterOverlays::RasterOverlayTileProvider& placeholder,
      Tile& tile,
      std::vector<CesiumGeospatial::Projection>& missingProjections,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

private:
  void computeTranslationAndScale(const Tile& tile);

  CesiumUtility::IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile>
      _pLoadingTile;
  CesiumUtility::IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile>
      _pReadyTile;
  int32_t _textureCoordinateID;
  glm::dvec2 _translation;
  glm::dvec2 _scale;
  AttachmentState _state;
  bool _originalFailed;
};

} // namespace Cesium3DTilesSelection
