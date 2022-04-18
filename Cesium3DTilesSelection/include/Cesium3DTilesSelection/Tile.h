#pragma once

#include "BoundingVolume.h"
#include "Library.h"
#include "RasterMappedTo3DTile.h"
#include "RasterOverlayTile.h"
#include "TileContext.h"
#include "TileID.h"
#include "TileRefine.h"
#include "TileSelectionState.h"

#include <Cesium3DTilesSelection/Exp_TileContent.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumUtility/DoublyLinkedList.h>

#include <glm/common.hpp>
#include <glm/mat4x4.hpp>
#include <gsl/span>

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
class Tileset;
struct TileContentLoadResult;

/**
 * @brief A tile in a {@link Tileset}.
 *
 * The tiles of a tileset form a hierarchy, where each tile may contain
 * renderable content, and each tile has an associated bounding volume.
 *
 * The actual hierarchy is represented with the {@link Tile::getParent}
 * and {@link Tile::getChildren} functions.
 *
 * The renderable content is provided as a {@link TileContentLoadResult}
 * from the {@link Tile::getContent} function.
 * The {@link Tile::getGeometricError} function returns the geometric
 * error of the representation of the renderable content of a tile.
 *
 * The {@link BoundingVolume} is given by the {@link Tile::getBoundingVolume}
 * function. This bounding volume encloses the renderable content of the
 * tile itself, as well as the renderable content of all children, yielding
 * a spatially coherent hierarchy of bounding volumes.
 *
 * The bounding volume of the content of an individual tile is given
 * by the {@link Tile::getContentBoundingVolume} function.
 *
 */
class CESIUM3DTILESSELECTION_API Tile final {
public:
  /**
   * The current state of this tile in the loading process.
   */
  enum class LoadState {
    /**
     * @brief This tile is in the process of being destroyed.
     *
     * Any pointers to it will soon be invalid.
     */
    Destroying = -3,

    /**
     * @brief Something went wrong while loading this tile and it will not be
     * retried.
     */
    Failed = -2,

    /**
     * @brief Something went wrong while loading this tile, but it may be a
     * temporary problem.
     */
    FailedTemporarily = -1,

    /**
     * @brief The tile is not yet loaded at all, beyond the metadata in
     * tileset.json.
     */
    Unloaded = 0,

    /**
     * @brief The tile content is currently being loaded.
     *
     * Note that while a tile is in this state, its {@link Tile::getContent},
     * and {@link Tile::getState}, methods may be called from the load thread,
     * and the state may change due to the internal loading process.
     */
    ContentLoading = 1,

    /**
     * @brief The tile content has finished loading.
     */
    ContentLoaded = 2,

    /**
     * @brief The tile is completely done loading.
     */
    Done = 3
  };

  /**
   * @brief Default constructor for an empty, uninitialized tile.
   */
  Tile() noexcept;

  /**
   * @brief Default destructor, which clears all resources associated with this
   * tile.
   */
  ~Tile();

  /**
   * @brief Copy constructor.
   *
   * @param rhs The other instance.
   */
  Tile(Tile& rhs) noexcept = delete;

  /**
   * @brief Move constructor.
   *
   * @param rhs The other instance.
   */
  Tile(Tile&& rhs) noexcept;

  /**
   * @brief Move assignment operator.
   *
   * @param rhs The other instance.
   */
  Tile& operator=(Tile&& rhs) noexcept;

  /**
   * @brief Returns the {@link Tileset} to which this tile belongs.
   */
  Tileset* getTileset() noexcept { return this->_pContext->pTileset; }

  /** @copydoc Tile::getTileset() */
  const Tileset* getTileset() const noexcept {
    return this->_pContext->pTileset;
  }

  /**
   * @brief Returns the {@link TileContext} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @return The tile context.
   */
  TileContext* getContext() noexcept { return this->_pContext; }

  /** @copydoc Tile::getContext() */
  const TileContext* getContext() const noexcept { return this->_pContext; }

  /**
   * @brief Set the {@link TileContext} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param pContext The tile context.
   */
  void setContext(TileContext* pContext) noexcept {
    this->_pContext = pContext;
  }

  /**
   * @brief Returns the parent of this tile in the tile hierarchy.
   *
   * This will be the `nullptr` if this is the root tile.
   *
   * @return The parent.
   */
  Tile* getParent() noexcept { return this->_pParent; }

  /** @copydoc Tile::getParent() */
  const Tile* getParent() const noexcept { return this->_pParent; }

  /**
   * @brief Set the parent of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param pParent The parent tile .
   */
  void setParent(Tile* pParent) noexcept { this->_pParent = pParent; }

  /**
   * @brief Returns a *view* on the children of this tile.
   *
   * The returned span will become invalid when this tile is destroyed.
   *
   * @return The children of this tile.
   */
  gsl::span<Tile> getChildren() noexcept {
    return gsl::span<Tile>(this->_children);
  }

  /** @copydoc Tile::getChildren() */
  gsl::span<const Tile> getChildren() const noexcept {
    return gsl::span<const Tile>(this->_children);
  }

  /**
   * @brief Allocates space for the given number of child tiles.
   *
   * This function is not supposed to be called by clients.
   *
   * @param count The number of child tiles.
   * @throws `std::runtime_error` if this tile already has children.
   */
  void createChildTiles(size_t count);

  /**
   * @brief Assigns the given child tiles to this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param children The child tiles.
   * @throws `std::runtime_error` if this tile already has children.
   */
  void createChildTiles(std::vector<Tile>&& children);

  /**
   * @brief Returns the {@link BoundingVolume} of this tile.
   *
   * This is a bounding volume that encloses the content of this tile,
   * as well as the content of all child tiles.
   *
   * @see Tile::getContentBoundingVolume
   *
   * @return The bounding volume.
   */
  const BoundingVolume& getBoundingVolume() const noexcept {
    return this->_boundingVolume;
  }

  /**
   * @brief Set the {@link BoundingVolume} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param value The bounding volume.
   */
  void setBoundingVolume(const BoundingVolume& value) noexcept {
    this->_boundingVolume = value;
  }

  /**
   * @brief Returns the viewer request volume of this tile.
   *
   * The viewer request volume is an optional {@link BoundingVolume} that
   * may be associated with a tile. It allows controlling the rendering
   * process of the tile content: If the viewer request volume is present,
   * then the content of the tile will only be rendered when the viewer
   * (i.e. the camera position) is inside the viewer request volume.
   *
   * @return The viewer request volume, or an empty optional.
   */
  const std::optional<BoundingVolume>& getViewerRequestVolume() const noexcept {
    return this->_viewerRequestVolume;
  }

  /**
   * @brief Set the viewer request volume of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param value The viewer request volume.
   */
  void
  setViewerRequestVolume(const std::optional<BoundingVolume>& value) noexcept {
    this->_viewerRequestVolume = value;
  }

  /**
   * @brief Returns the geometric error of this tile.
   *
   * This is the error, in meters, introduced if this tile is rendered and its
   * children are not. This is used to compute screen space error, i.e., the
   * error measured in pixels.
   *
   * @return The geometric error of this tile, in meters.
   */
  double getGeometricError() const noexcept { return this->_geometricError; }

  /**
   * @brief Set the geometric error of the contents of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param value The geometric error, in meters.
   */
  void setGeometricError(double value) noexcept {
    this->_geometricError = value;
  }

  /**
   * @brief Gets the tile's geometric error as if by calling
   * {@link getGeometricError}, except that if the error is smaller than
   * {@link Math::Epsilon5} the returned geometric error is instead computed as
   * half of the parent tile's (non-zero) geometric error.
   *
   * This is useful for determining when to refine what would ordinarily be a
   * leaf tile, for example to attach more detailed raster overlays to it.
   *
   * If this tile and all of its ancestors have a geometric error less than
   * {@link Math::Epsilon5}, returns {@link Math::Epsilon5}.
   *
   * @return The non-zero geometric error.
   */
  double getNonZeroGeometricError() const noexcept;

  /**
   * @brief Returns whether to unconditionally refine this tile.
   *
   * This is useful in cases such as with external tilesets, where instead of a
   * tile having any content, it points to an external tileset's root. So the
   * tile always needs to be refined otherwise the external tileset will not be
   * displayed.
   *
   * @return Whether to uncoditionally refine this tile.
   */
  bool getUnconditionallyRefine() const noexcept {
    return glm::isinf(this->_geometricError);
  }

  /**
   * @brief Marks that this tile should be unconditionally refined.
   *
   * This function is not supposed to be called by clients.
   */
  void setUnconditionallyRefine() noexcept {
    this->_geometricError = std::numeric_limits<double>::infinity();
  }

  /**
   * @brief The refinement strategy of this tile.
   *
   * Returns the {@link TileRefine} value that indicates the refinement strategy
   * for this tile. This is `Add` when the content of the
   * child tiles is *added* to the content of this tile during refinement, and
   * `Replace` when the content of the child tiles *replaces*
   * the content of this tile during refinement.
   *
   * @return The refinement strategy.
   */
  TileRefine getRefine() const noexcept { return this->_refine; }

  /**
   * @brief Set the refinement strategy of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param value The refinement strategy.
   */
  void setRefine(TileRefine value) noexcept { this->_refine = value; }

  /**
   * @brief Gets the transformation matrix for this tile.
   *
   * This matrix does _not_ need to be multiplied with the tile's parent's
   * transform as this has already been done.
   *
   * @return The transform matrix.
   */
  const glm::dmat4x4& getTransform() const noexcept { return this->_transform; }

  /**
   * @brief Set the transformation matrix for this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param value The transform matrix.
   */
  void setTransform(const glm::dmat4x4& value) noexcept {
    this->_transform = value;
  }

  /**
   * @brief Returns the {@link TileID} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @return The tile ID.
   */
  const TileID& getTileID() const noexcept { return this->_id; }

  /**
   * @brief Set the {@link TileID} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param id The tile ID.
   */
  void setTileID(const TileID& id) noexcept;

  /**
   * @brief Returns the {@link BoundingVolume} of the renderable content of this
   * tile.
   *
   * The content bounding volume is a bounding volume that tightly fits only the
   * renderable content of the tile. This enables tighter view frustum culling,
   * making it possible to exclude from rendering any content not in the view
   * frustum.
   *
   * @see Tile::getBoundingVolume
   */
  const std::optional<BoundingVolume>&
  getContentBoundingVolume() const noexcept {
    return this->_contentBoundingVolume;
  }

  /**
   * @brief Set the {@link BoundingVolume} of the renderable content of this
   * tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param value The content bounding volume
   */
  void setContentBoundingVolume(
      const std::optional<BoundingVolume>& value) noexcept {
    this->_contentBoundingVolume = value;
  }

  /**
   * @brief Returns the {@link TileContentLoadResult} for the content of this
   * tile.
   *
   * This will be a `nullptr` if the content of this tile has not yet been
   * loaded, as indicated by the indicated by the {@link Tile::getState} of this
   * tile not being {@link Tile::LoadState::ContentLoaded}.
   *
   * @return The tile content load result, or `nullptr` if no content is loaded
   */
  TileContentLoadResult* getContent() noexcept { return this->_pContent.get(); }

  /** @copydoc Tile::getContent() */
  const TileContentLoadResult* getContent() const noexcept {
    return this->_pContent.get();
  }

  /**
   * @brief Sets the {@link TileContentLoadResult} of this tile to be an empty
   * object instead of nullptr.
   *
   * This is useful to indicate to the traversal that this tile points to an
   * external or implicit tileset.
   */
  void setEmptyContent() noexcept;

  /**
   * @brief Returns internal resources required for rendering this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @return The renderer resources.
   */
  void* getRendererResources() const noexcept {
    return this->_pRendererResources;
  }

  /**
   * @brief Returns the {@link LoadState} of this tile.
   */
  LoadState getState() const noexcept;

  /**
   * @brief Set the {@link LoadState} of this tile.
   *
   * Not to be called by clients.
   */
  void setState(LoadState value) noexcept;

  /**
   * @brief Returns the {@link TileSelectionState} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @return The last selection state
   */
  TileSelectionState& getLastSelectionState() noexcept {
    return this->_lastSelectionState;
  }

  /** @copydoc Tile::getLastSelectionState() */
  const TileSelectionState& getLastSelectionState() const noexcept {
    return this->_lastSelectionState;
  }

  /**
   * @brief Set the {@link TileSelectionState} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param newState The new stace
   */
  void setLastSelectionState(const TileSelectionState& newState) noexcept {
    this->_lastSelectionState = newState;
  }

  /**
   * @brief Returns the raster overlay tiles that have been mapped to this tile.
   */
  std::vector<RasterMappedTo3DTile>& getMappedRasterTiles() noexcept {
    return this->_rasterTiles;
  }

  /** @copydoc Tile::getMappedRasterTiles() */
  const std::vector<RasterMappedTo3DTile>&
  getMappedRasterTiles() const noexcept {
    return this->_rasterTiles;
  }

  /**
   * @brief Determines if this tile is currently renderable.
   */
  bool isRenderable() const noexcept;

  /**
   * @brief Determines if this tile is has external tileset content.
   */
  bool isExternalTileset() const noexcept;

  /**
   * @brief Trigger the process of loading the {@link Tile::getContent}.
   *
   * This function is not supposed to be called by clients.
   *
   * Do NOT call this function if the tile does not have content to load and
   * does not need to be upsampled.
   *
   * If this tile is not in {@link Tile::LoadState::Unloaded}, any previously
   * throttled rasters will be reloaded.
   *
   * Otherwise, if this is a non-upsampled tile:
   * - The tile will be put into the {@link Tile::LoadState::ContentLoading}
   *   state, the content will be requested, and then be processed
   *   asynchronously.
   *
   * Otherwise, if this is an upsampled tile, this tile's content needs to be
   * derived from its parent:
   * - If the parent has a load state of {@link Tile::LoadState::Done}, we will
   *   asynchronously upsample from it to load this tile's content.
   * - If the parent has any other load state, we will call
   *   {@link Tile::loadContent} for the parent and return after setting this
   *   tile back to {@link Tile::Unloaded}.
   *
   * Once the asynchronous content loading or upsampling is done, the tile's
   * state will be set to {@link Tile::LoadState::ContentLoaded}. If we are
   * waiting on a parent tile to be able to upsample, the state will be set to
   * {@link Tile::LoadState::Unloaded}.
   */
  void loadContent();

  /**
   * @brief Finalizes the tile from the loaded content.
   *
   * Once the tile is {@link Tile::LoadState::ContentLoaded} after the
   * asynchronous {@link Tile::loadContent} finishes, this should be called to
   * finalize the tile from the loaded content. Nothing happens if this tile is
   * not in {@link Tile::LoadState::ContentLoaded}. After this is called, the
   * tile will be set to {@link Tile::LoadState::Done}.
   */
  void processLoadedContent();

  /**
   * @brief Frees all resources that have been allocated for the
   * {@link Tile::getContent}.
   *
   * This function is not supposed to be called by clients.
   *
   * If the operation for loading the tile content is currently in progress, as
   * indicated by the {@link Tile::getState} of this tile being
   * {@link Tile::LoadState::ContentLoading}), then nothing will be done,
   * and `false` will be returned.
   *
   * Otherwise, the resources that have been allocated for the tile content will
   * be freed.
   *
   * @return Whether the content was unloaded.
   */
  bool unloadContent() noexcept;

  /**
   * @brief Gives this tile a chance to update itself each render frame.
   *
   * @param previousFrameNumber The number of the previous render frame.
   * @param currentFrameNumber The number of the current render frame.
   */
  void update(int32_t previousFrameNumber, int32_t currentFrameNumber);

  /**
   * @brief Marks the tile as permanently failing to load.
   *
   * This function is not supposed to be called by clients.
   *
   * Moves the tile from the `FailedTemporarily` state to the `Failed` state.
   * If the tile is not in the `FailedTemporarily` state, this method does
   * nothing.
   */
  void markPermanentlyFailed() noexcept;

  /**
   * @brief Determines the number of bytes in this tile's geometry and texture
   * data.
   */
  int64_t computeByteSize() const noexcept;

  /*****************************************************
   *
   * Experimental methods.
   *
   *****************************************************/
  void exp_SetContent(std::unique_ptr<TileContent>&& pContent) noexcept {
    exp_pContent = std::move(pContent);
  }

  const TileContent* exp_GetContent() const noexcept {
    return exp_pContent.get();
  }

  TileContent* exp_GetContent() noexcept { return exp_pContent.get(); }

private:
  /**
   * @brief Upsample the parent of this tile.
   *
   * This method should only be called when this tile's parent is already
   * loaded.
   */
  void upsampleParent(std::vector<CesiumGeospatial::Projection>&& projections);

  // Position in bounding-volume hierarchy.
  TileContext* _pContext;
  Tile* _pParent;
  std::vector<Tile> _children;

  // Properties from tileset.json.
  // These are immutable after the tile leaves TileState::Unloaded.
  BoundingVolume _boundingVolume;
  std::optional<BoundingVolume> _viewerRequestVolume;
  double _geometricError;
  TileRefine _refine;
  glm::dmat4x4 _transform;

  TileID _id;
  std::optional<BoundingVolume> _contentBoundingVolume;

  // Load state and data.
  std::atomic<LoadState> _state;
  std::unique_ptr<TileContentLoadResult> _pContent;
  void* _pRendererResources;

  // Selection state
  TileSelectionState _lastSelectionState;

  // Overlays
  std::vector<RasterMappedTo3DTile> _rasterTiles;

  CesiumUtility::DoublyLinkedListPointers<Tile> _loadedTilesLinks;

  /*****************************************************
   *
   * Experimental Data.
   *
   *****************************************************/
  std::unique_ptr<TileContent> exp_pContent;

public:
  /**
   * @brief A {@link CesiumUtility::DoublyLinkedList} for tile objects.
   */
  typedef CesiumUtility::DoublyLinkedList<Tile, &Tile::_loadedTilesLinks>
      LoadedLinkedList;
};

} // namespace Cesium3DTilesSelection
