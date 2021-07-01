#pragma once

#include "Cesium3DTiles/RasterOverlayTile.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumUtility/IntrusivePointer.h"
#include <vector>
#include <memory>

namespace Cesium3DTiles {

class Tile;
class RasterMappedTo3DTile;


/**
 * @brief A {@link RasterOverlayTile} that will be combined with others to
 * form a single output texture.
 */
class RasterToCombine final {
public:
  /**
   * @brief Creates a new instance.
   * 
   * @param pRasterTile The {@link RasterOverlayTile} to combine with others.
   * @param textureCoordinateRectangle The texture coordinate rectangle that 
   * indicates the region that is covered by the raster overlay tile.
   */
  RasterToCombine(
      const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile,
      const CesiumGeometry::Rectangle textureCoordinateRectangle);

  // TODO add back doxygen comments here

  const CesiumUtility::IntrusivePointer<RasterOverlayTile>& getLoadingTile() const {
    return this->_pLoadingTile;
  }

  CesiumUtility::IntrusivePointer<RasterOverlayTile>& getLoadingTile() {
    return this->_pLoadingTile;
  }

  const CesiumUtility::IntrusivePointer<RasterOverlayTile>& getReadyTile() const {
    return this->_pReadyTile;
  }
  
  CesiumUtility::IntrusivePointer<RasterOverlayTile>& getReadyTile() {
    return this->_pReadyTile;
  }

  const CesiumGeometry::Rectangle& getTextureCoordinateRectangle() const {
    return this->_textureCoordinateRectangle;
  }

  const glm::dvec2& getTranslation() const {
    return this->_translation;
  }

  const glm::dvec2& getScale() const {
    return this->_scale;
  }

private:
  CesiumUtility::IntrusivePointer<RasterOverlayTile> _pLoadingTile;
  CesiumUtility::IntrusivePointer<RasterOverlayTile> _pReadyTile;
  CesiumGeometry::Rectangle _textureCoordinateRectangle;
  glm::dvec2 _translation;
  glm::dvec2 _scale;
  bool _originalFailed;

  friend class RasterMappedTo3DTile;
};

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
   * @param rastersToCombine The raster tiles that need to be combined together
   * to form the final output texture.
   */
  RasterMappedTo3DTile(const std::vector<RasterToCombine>& rastersToCombine);

  /**
   * @brief Returns the list of rasters that are to be combined and mapped to 
   * the geometry tile.
   * 
   * @return The list of raster tiles to be combined.
   */
  const std::vector<RasterToCombine> getRastersToCombine() const {
    return this->_rastersToCombine;
  }

  /**
   * @copydoc getRastersToCombine 
   */
  std::vector<RasterToCombine> getRastersToCombine() {
    return this->_rastersToCombine;
  }

  /**
   * @brief Returns an identifier for the texture coordinates of this tile.
   *
   * This is an unspecified identifier that is used internally by the
   * tile rendering infrastructure, to identify the texture coordinates
   * in the rendering environment.
   *
   * @return The texture coordinate ID.
   */
  uint32_t getTextureCoordinateID() const noexcept {
    return this->_textureCoordinateID;
  }

  /**
   * @brief Sets the texture coordinate ID.
   *
   * @see getTextureCoordinateID
   *
   * @param textureCoordinateID The ID.
   */
  void setTextureCoordinateID(uint32_t textureCoordinateID) noexcept {
    this->_textureCoordinateID = textureCoordinateID;
  }

  /**
   * @brief The texture coordinate range in which the raster is applied.
   *
   * This is part of a unit rectangle, where the rectangle
   * `(minimumX, minimumY, maximumX, maximumY)` corresponds
   * to the `(west, south, east, north)` of the tile region,
   * and each coordinate is in the range `[0,1]`.
   *
   * @return The texture coordinate rectangle.
   */
  const CesiumGeometry::Rectangle&
  getTextureCoordinateRectangle() const noexcept {
    return this->_textureCoordinateRectangle;
  }

  /**
   * @brief Returns the {@link AttachmentState}.
   *
   * This function is not supposed to be called by clients.
   */
  AttachmentState getState() const noexcept { return this->_state; }

  /**
   * @brief Returns the {@link RasterOverlayTile} that is the combination of
   * all provided rasters.
   * 
   * @return The combined raster overlay tile. 
   */
  const std::shared_ptr<RasterOverlayTile>& getCombinedTile() const noexcept {
    return this->_combinedTile;
  }

  /**
   * @brief Tile availability states.
   *
   * Values of this enumeration are returned by {@link update}, which
   * in turn is called by {@link Tile::update}. These values are used
   * to determine whether a leaf tile has been reached, but the
   * associated raster tiles are not yet the most detailed ones that
   * are available.
   */
  enum class MoreDetailAvailable {

    /** @brief There are no more detailed raster tiles. */
    No = 0,

    /** @brief There are more detailed raster tiles. */
    Yes = 1,

    /** @brief It is not known whether more detailed raster tiles are available.
     */
    Unknown = 2
  };

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
  MoreDetailAvailable update(Tile& tile);

  /**
   * @brief Detach the raster from the given tile.
   */
  void detachFromTile(Tile& tile) noexcept;

  // void attachToTile(Tile& tile);

  /**
   * @brief Whether any of the rasters-to-combine have a loading tile.
   */
  bool anyLoading() const noexcept;

  /**
   * @brief Whether all of the rasters-to-combine have a ready tile. 
   */
  bool allReady() const noexcept;

  /**
   * @brief Whether any of the loading tiles are placeholders
   */
  bool hasPlaceholder() const noexcept;

private:
  static void computeTranslationAndScale(
      RasterToCombine& rasterToCombine,
      const Tile& tile);
  std::optional<CesiumGltf::ImageCesium> blitRasters();

  std::vector<RasterToCombine> _rastersToCombine;
  uint32_t _textureCoordinateID;
  CesiumGeometry::Rectangle _textureCoordinateRectangle;
  AttachmentState _state;
  std::shared_ptr<RasterOverlayTile> _combinedTile;
};

} // namespace Cesium3DTiles
