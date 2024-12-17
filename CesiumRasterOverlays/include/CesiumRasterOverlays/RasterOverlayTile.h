#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <vector>

namespace CesiumUtility {
struct Credit;
}

namespace CesiumRasterOverlays {

class RasterOverlay;
class RasterOverlayTileProvider;

/**
 * @brief Raster image data for a tile in a quadtree.
 *
 * Instances of this clas represent tiles of a quadtree that have
 * an associated image, which us used as an imagery overlay
 * for tile geometry. The connection between the imagery data
 * and the actual tile geometry is established via the
 * {@link Cesium3DTilesSelection::RasterMappedTo3DTile} class, which combines a
 * raster overlay tile with texture coordinates, to map the
 * image on the geometry of a {@link Cesium3DTilesSelection::Tile}.
 */
class RasterOverlayTile final
    : public CesiumUtility::ReferenceCountedNonThreadSafe<RasterOverlayTile> {
public:
  /**
   * @brief Lifecycle states of a raster overlay tile.
   */
  enum class LoadState {
    /**
     * @brief Indicator for a placeholder tile.
     */
    Placeholder = -2,

    /**
     * @brief The image request or image creation failed.
     */
    Failed = -1,

    /**
     * @brief The initial state
     */
    Unloaded = 0,

    /**
     * @brief The request for loading the image data is still pending.
     */
    Loading = 1,

    /**
     * @brief The image data has been loaded and the image has been created.
     */
    Loaded = 2,

    /**
     * @brief The rendering resources for the image data have been created.
     */
    Done = 3
  };

  /**
   * @brief Tile availability states.
   *
   * Values of this enumeration are returned by
   * {@link Cesium3DTilesSelection::RasterMappedTo3DTile::update}, which in turn is called by
   * `TilesetContentManager::updateDoneState`. These values are used to
   * determine whether a leaf tile has been reached, but the associated raster
   * tiles are not yet the most detailed ones that are available.
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
   * @brief Constructs a placeholder tile for the tile provider.
   *
   * The {@link getState} of this instance will always be
   * {@link LoadState::Placeholder}.
   *
   * @param tileProvider The {@link RasterOverlayTileProvider}. This object
   * _must_ remain valid for the entire lifetime of the tile. If the tile
   * provider is destroyed before the tile, undefined behavior will result.
   */
  RasterOverlayTile(RasterOverlayTileProvider& tileProvider) noexcept;

  /**
   * @brief Creates a new instance.
   *
   * The tile will start in the `Unloaded` state, and will not begin loading
   * until {@link RasterOverlayTileProvider::loadTile} or
   * {@link RasterOverlayTileProvider::loadTileThrottled} is called.
   *
   * @param tileProvider The {@link RasterOverlayTileProvider}. This object
   * _must_ remain valid for the entire lifetime of the tile. If the tile
   * provider is destroyed before the tile, undefined behavior may result.
   * @param targetScreenPixels The maximum number of pixels on the screen that
   * this tile is meant to cover. The overlay image should be approximately this
   * many pixels divided by the
   * {@link RasterOverlayOptions::maximumScreenSpaceError} in order to achieve
   * the desired level-of-detail, but it does not need to be exactly this size.
   * @param imageryRectangle The rectangle that the returned image must cover.
   * It is allowed to cover a slightly larger rectangle in order to maintain
   * pixel alignment. It may also cover a smaller rectangle when the overlay
   * itself does not cover the entire rectangle.
   */
  RasterOverlayTile(
      RasterOverlayTileProvider& tileProvider,
      const glm::dvec2& targetScreenPixels,
      const CesiumGeometry::Rectangle& imageryRectangle) noexcept;

  /** @brief Default destructor. */
  ~RasterOverlayTile();

  /**
   * @brief Returns the {@link RasterOverlayTileProvider} that created this instance.
   */
  RasterOverlayTileProvider& getTileProvider() noexcept {
    return *this->_pTileProvider;
  }

  /**
   * @brief Returns the {@link RasterOverlayTileProvider} that created this instance.
   */
  const RasterOverlayTileProvider& getTileProvider() const noexcept {
    return *this->_pTileProvider;
  }

  /**
   * @brief Returns the {@link RasterOverlay} that created this instance.
   */
  RasterOverlay& getOverlay() noexcept;

  /**
   * @brief Returns the {@link RasterOverlay} that created this instance.
   */
  const RasterOverlay& getOverlay() const noexcept;

  /**
   * @brief Returns the {@link CesiumGeometry::Rectangle} that defines the bounds
   * of this tile in the raster overlay's projected coordinates.
   */
  const CesiumGeometry::Rectangle& getRectangle() const noexcept {
    return this->_rectangle;
  }

  /**
   * @brief Gets the number of screen pixels in each direction that should be
   * covered by this tile's texture.
   *
   * This is used to control which content (how highly detailed) the
   * {@link RasterOverlayTileProvider} uses within the bounds of this tile.
   */
  glm::dvec2 getTargetScreenPixels() const noexcept {
    return this->_targetScreenPixels;
  }

  /**
   * @brief Returns the current {@link LoadState}.
   */
  LoadState getState() const noexcept { return this->_state; }

  /**
   * @brief Returns the list of \ref CesiumUtility::Credit "Credit"s needed for
   * this tile.
   */
  const std::vector<CesiumUtility::Credit>& getCredits() const noexcept {
    return this->_tileCredits;
  }

  /**
   * @brief Returns the image data for the tile.
   *
   * This will only contain valid image data if the {@link getState} of
   * this tile is {@link LoadState `Loaded`} or {@link LoadState `Done`}.
   *
   * @return The image data.
   */
  CesiumUtility::IntrusivePointer<const CesiumGltf::ImageAsset>
  getImage() const noexcept {
    return this->_pImage;
  }

  /**
   * @brief Returns the image data for the tile.
   *
   * This will only contain valid image data if the {@link getState} of
   * this tile is {@link LoadState `Loaded`} or {@link LoadState `Done`}.
   *
   * @return The image data.
   */
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> getImage() noexcept {
    return this->_pImage;
  }

  /**
   * @brief Create the renderer resources for the loaded image.
   *
   * If the {@link getState} of this tile is not {@link LoadState `Loaded`},
   * then nothing will be done. Otherwise, the renderer resources will be
   * prepared, so that they may later be obtained with
   * {@link getRendererResources}, and the {@link getState} of this tile
   * will change to {@link LoadState `Done`}.
   */
  void loadInMainThread();

  /**
   * @brief Returns the renderer resources that have been created for this tile.
   */
  void* getRendererResources() const noexcept {
    return this->_pRendererResources;
  }

  /**
   * @brief Set the renderer resources for this tile.
   *
   * This function is not supposed to be called by clients.
   */
  void setRendererResources(void* pValue) noexcept {
    this->_pRendererResources = pValue;
  }

  /**
   * @brief Determines if more detailed data is available for the spatial area
   * covered by this tile.
   */
  MoreDetailAvailable isMoreDetailAvailable() const noexcept {
    return this->_moreDetailAvailable;
  }

private:
  friend class RasterOverlayTileProvider;

  void setState(LoadState newState) noexcept;

  // This is a raw pointer instead of an IntrusivePointer in order to avoid
  // circular references, particularly among a placeholder tile provider and
  // placeholder tile. However, to avoid undefined behavior, the tile provider
  // is required to outlive the tile. In normal use, the RasterOverlayCollection
  // ensures that this is true.
  RasterOverlayTileProvider* _pTileProvider;
  glm::dvec2 _targetScreenPixels;
  CesiumGeometry::Rectangle _rectangle;
  std::vector<CesiumUtility::Credit> _tileCredits;
  LoadState _state;
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> _pImage;
  void* _pRendererResources;
  MoreDetailAvailable _moreDetailAvailable;
};
} // namespace CesiumRasterOverlays
