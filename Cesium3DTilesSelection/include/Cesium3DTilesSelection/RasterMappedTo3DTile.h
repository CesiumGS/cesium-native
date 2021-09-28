#pragma once

#include "Cesium3DTilesSelection/RasterOverlayTile.h"

#include <CesiumGeometry/Rectangle.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <memory>

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief The result of applying a {@link RasterOverlayTile} to geometry.
 *
 * Instances of this class are used by a {@link Tile} in order to map
 * imagery data that is given as {@link RasterOverlayTile} instances
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
   * @param pRasterTile The {@link RasterOverlayTile} that is mapped to the
   * geometry.
   */
  RasterMappedTo3DTile(
      const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile);

  /**
   * @brief Returns a {@link RasterOverlayTile} that is currently loading.
   *
   * The caller has to check the exact state of this tile, using
   * {@link Tile::getState}.
   *
   * @return The tile that is loading, or `nullptr`.
   */
  RasterOverlayTile* getLoadingTile() noexcept {
    return this->_pLoadingTile.get();
  }

  /** @copydoc getLoadingTile */
  const RasterOverlayTile* getLoadingTile() const noexcept {
    return this->_pLoadingTile.get();
  }

  /**
   * @brief Returns the {@link RasterOverlayTile} that represents the imagery
   * data that is ready to render.
   *
   * This will be `nullptr` when the tile data has not yet been loaded.
   *
   * @return The tile, or `nullptr`.
   */
  RasterOverlayTile* getReadyTile() noexcept { return this->_pReadyTile.get(); }

  /** @copydoc getReadyTile */
  const RasterOverlayTile* getReadyTile() const noexcept {
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
   * This is only supposed to be called by {@link Tile::update}. It
   * will return whether there is a more detailed version of the
   * raster data available.
   *
   * @param tile The owner tile.
   * @return The {@link MoreDetailAvailable} state.
   */
  RasterOverlayTile::MoreDetailAvailable update(Tile& tile);

  /**
   * @brief Detach the raster from the given tile.
   */
  void detachFromTile(Tile& tile) noexcept;

private:
  void computeTranslationAndScale(const Tile& tile);

  CesiumUtility::IntrusivePointer<RasterOverlayTile> _pLoadingTile;
  CesiumUtility::IntrusivePointer<RasterOverlayTile> _pReadyTile;
  int32_t _textureCoordinateID;
  glm::dvec2 _translation;
  glm::dvec2 _scale;
  AttachmentState _state;
  bool _originalFailed;
};

} // namespace Cesium3DTilesSelection
