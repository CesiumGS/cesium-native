#pragma once

#include "Cesium3DTiles/TileID.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumGltf/Model.h"
#include <vector>

namespace Cesium3DTiles {

struct Credit;
class RasterOverlay;
class RasterOverlayTileProvider;

/**
 * @brief Raster image data for a tile in a quadtree.
 *
 * Instances of this clas represent tiles of a quadtree that have
 * an associated image, which us used as an imagery overlay
 * for tile geometry. The connection between the imagery data
 * and the actual tile geometry is established via the
 * {@link RastersMappedTo3DTile} class, which combines a
 * raster overlay tile with texture coordinates, to map the
 * image on the geometry of a {@link Tile}.
 */
class RasterOverlayTile final {
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
   * @brief Constructs a placeholder tile for the tile provider.
   *
   * The {@link getState} of this instance will always be
   * {@link LoadState::Placeholder}.
   *
   * @param overlay The {@link RasterOverlay}.
   */
  RasterOverlayTile(RasterOverlay& overlay) noexcept;

  /**
   * @brief Creates a new instance.
   *
   * This is called by a {@link RasterOverlayTileProvider} when a new,
   * previously unknown tile is reqested. It receives the request for the image
   * data, and the {@link getState} will initially be
   * {@link LoadState `Loading`}.
   * The constructor will attach a callback to this request.  When
   * the request completes successfully and the {@link getImage} data can be
   * created, the state of this instance will change to
   * {@link LoadState `Loaded`}.
   * Otherwise, the state will become {@link LoadState `Failed`}.
   *
   * @param overlay The {@link RasterOverlay}.
   * @param tileID The {@link TileID} for this tile.
   * @param imageryRectangle The {@link CesiumGeometry::Rectangle} that defines
   * the imagery rectange for this tile.
   */
  RasterOverlayTile(
      RasterOverlay& overlay,
      double targetGeometricError,
      const CesiumGeometry::Rectangle& imageryRectangle);

  /** @brief Default destructor. */
  ~RasterOverlayTile();

  /**
   * @brief Returns the {@link RasterOverlay} of this instance.
   */
  RasterOverlay& getOverlay() noexcept { return *this->_pOverlay; }

  /**
   * @brief Returns the {@link RasterOverlay} of this instance.
   */
  const RasterOverlay& getOverlay() const noexcept { return *this->_pOverlay; }

  /**
   * @brief Returns the {@link CesiumGeometry::Rectangle} that defines the bounds
   * of this tile in the raster overlay's projected coordinates.
   */
  const CesiumGeometry::Rectangle& getRectangle() const {
    return this->_rectangle;
  }

  double getTargetGeometricError() const {
    return this->_targetGeometricError;
  }

  /**
   * @brief Returns the current {@link LoadState}.
   */
  LoadState getState() const noexcept { return this->_state; }

  /**
   * @brief Returns the list of {@link Credit}s needed for this tile.
   */
  const std::vector<Credit>& getCredits() const noexcept {
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
  const CesiumGltf::ImageCesium& getImage() const noexcept {
    return this->_image;
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
  void* getRendererResources() const { return this->_pRendererResources; }

  /**
   * @brief Set the renderer resources for this tile.
   *
   * This function is not supposed to be called by clients.
   */
  void setRendererResources(void* pValue) {
    this->_pRendererResources = pValue;
  }

  MoreDetailAvailable isMoreDetailAvailable() const { return this->_moreDetailAvailable; }

  /**
   * @brief Adds a counted reference to this instance.
   */
  void addReference() noexcept;

  /**
   * @brief Removes a counted reference from this instance.
   */
  void releaseReference() noexcept;

  /**
   * @brief Returns the current reference count of this instance.
   */
  uint32_t getReferenceCount() const noexcept { return this->_references; }

private:
  friend class RasterOverlayTileProvider;
  friend class RastersMappedTo3DTile;

  void setState(LoadState newState);

  RasterOverlay* _pOverlay;
  double _targetGeometricError;
  CesiumGeometry::Rectangle _rectangle;
  std::vector<Credit> _tileCredits;
  LoadState _state;
  CesiumGltf::ImageCesium _image;
  void* _pRendererResources;
  uint32_t _references;
  MoreDetailAvailable _moreDetailAvailable;
};
} // namespace Cesium3DTiles
