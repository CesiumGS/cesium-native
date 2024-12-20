#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TileSelectionState.h>
#include <CesiumUtility/DoublyLinkedList.h>

#include <glm/common.hpp>

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
class TilesetContentLoader;

/**
 * The current state of this tile in the loading process.
 */
enum class TileLoadState {
  /**
   * @brief This tile is in the process of being unloaded, but could not be
   * fully unloaded because an asynchronous process is using its loaded data.
   */
  Unloading = -2,

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
  Done = 3,

  /**
   * @brief Something went wrong while loading this tile and it will not be
   * retried.
   */
  Failed = 4,
};

/**
 * @brief A tile in a {@link Tileset}.
 *
 * The tiles of a tileset form a hierarchy, where each tile may contain
 * renderable content, and each tile has an associated bounding volume.
 *
 * The actual hierarchy is represented with the {@link Tile::getParent}
 * and {@link Tile::getChildren} functions.
 *
 * The renderable content is provided as a {@link TileContent}
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
   * @brief Construct a tile with unknown content and a loader that is used to
   * load the content of this tile. Tile has Unloaded status when initializing
   * with this constructor.
   *
   * @param pLoader The {@link TilesetContentLoader} that is used to load the tile.
   */
  explicit Tile(TilesetContentLoader* pLoader) noexcept;

  /**
   * @brief Construct a tile with an external content and a loader that is
   * associated with this tile. Tile has ContentLoaded status when initializing
   * with this constructor.
   *
   * @param pLoader The {@link TilesetContentLoader} that is assiocated with this tile.
   * @param externalContent External content that is associated with this tile.
   */
  Tile(
      TilesetContentLoader* pLoader,
      std::unique_ptr<TileExternalContent>&& externalContent) noexcept;

  /**
   * @brief Construct a tile with an empty content and a loader that is
   * associated with this tile. Tile has ContentLoaded status when initializing
   * with this constructor.
   *
   * @param pLoader The {@link TilesetContentLoader} that is assiocated with this tile.
   * @param emptyContent A content tag indicating that the tile has no content.
   */
  Tile(TilesetContentLoader* pLoader, TileEmptyContent emptyContent) noexcept;

  /**
   * @brief Default destructor, which clears all resources associated with this
   * tile.
   */
  ~Tile() noexcept = default;

  /**
   * @brief Copy constructor.
   *
   * @param rhs The other instance.
   */
  Tile(const Tile& rhs) = delete;

  /**
   * @brief Move constructor.
   *
   * @param rhs The other instance.
   */
  Tile(Tile&& rhs) noexcept;

  /**
   * @brief Copy constructor.
   *
   * @param rhs The other instance.
   */
  Tile& operator=(const Tile& rhs) = delete;

  /**
   * @brief Move assignment operator.
   *
   * @param rhs The other instance.
   */
  Tile& operator=(Tile&& rhs) noexcept;

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
   * @brief Returns a *view* on the children of this tile.
   *
   * The returned span will become invalid when this tile is destroyed.
   *
   * @return The children of this tile.
   */
  std::span<Tile> getChildren() noexcept {
    return std::span<Tile>(this->_children);
  }

  /** @copydoc Tile::getChildren() */
  std::span<const Tile> getChildren() const noexcept {
    return std::span<const Tile>(this->_children);
  }

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
   * {@link CesiumUtility::Math::Epsilon5} the returned geometric error is instead computed as
   * half of the parent tile's (non-zero) geometric error.
   *
   * This is useful for determining when to refine what would ordinarily be a
   * leaf tile, for example to attach more detailed raster overlays to it.
   *
   * If this tile and all of its ancestors have a geometric error less than
   * {@link CesiumUtility::Math::Epsilon5}, returns {@link CesiumUtility::Math::Epsilon5}.
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
  void setTileID(const TileID& id) noexcept { this->_id = id; }

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
   * @brief Determines the number of bytes in this tile's geometry and texture
   * data.
   */
  int64_t computeByteSize() const noexcept;

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
   * @brief Get the content of the tile.
   */
  const TileContent& getContent() const noexcept { return _content; }

  /** @copydoc Tile::getContent() const */
  TileContent& getContent() noexcept { return _content; }

  /**
   * @brief Determines if this tile is currently renderable.
   */
  bool isRenderable() const noexcept;

  /**
   * @brief Determines if this tile has mesh content.
   */
  bool isRenderContent() const noexcept;

  /**
   * @brief Determines if this tile has external tileset content.
   */
  bool isExternalContent() const noexcept;

  /**
   * @brief Determines if this tile has empty content.
   */
  bool isEmptyContent() const noexcept;

  /**
   * @brief get the loader that is used to load the tile content.
   */
  TilesetContentLoader* getLoader() const noexcept;

  /**
   * @brief Returns the {@link TileLoadState} of this tile.
   */
  TileLoadState getState() const noexcept;

private:
  struct TileConstructorImpl {};
  template <
      typename... TileContentArgs,
      typename TileContentEnable = std::enable_if_t<
          std::is_constructible_v<TileContent, TileContentArgs&&...>,
          int>>
  Tile(
      TileConstructorImpl tag,
      TileLoadState loadState,
      TilesetContentLoader* pLoader,
      TileContentArgs&&... args);

  void setParent(Tile* pParent) noexcept;

  void setState(TileLoadState state) noexcept;

  /**
   * @brief Gets a flag indicating whether this tile might have latent children.
   * Latent children don't exist in the `_children` property, but can be created
   * by the {@link TilesetContentLoader}.
   *
   * When true, this tile might have children that can be created by the
   * TilesetContentLoader but aren't yet reflected in the `_children` property.
   * For example, in implicit tiling, we save memory by only creating explicit
   * Tile instances from implicit availability as those instances are needed.
   * When this flag is true, the creation of those explicit instances hasn't
   * happened yet for this tile.
   *
   * If this flag is false, the children have already been created, if they
   * exist. The tile may still have no children because it is a leaf node.
   */
  bool getMightHaveLatentChildren() const noexcept;

  void setMightHaveLatentChildren(bool mightHaveLatentChildren) noexcept;

  // Position in bounding-volume hierarchy.
  Tile* _pParent;
  std::vector<Tile> _children;

  // Properties from tileset.json.
  // These are immutable after the tile leaves TileState::Unloaded.
  TileID _id;
  BoundingVolume _boundingVolume;
  std::optional<BoundingVolume> _viewerRequestVolume;
  std::optional<BoundingVolume> _contentBoundingVolume;
  double _geometricError;
  TileRefine _refine;
  glm::dmat4x4 _transform;

  // Selection state
  TileSelectionState _lastSelectionState;

  // tile content
  CesiumUtility::DoublyLinkedListPointers<Tile> _loadedTilesLinks;
  TileContent _content;
  TilesetContentLoader* _pLoader;
  TileLoadState _loadState;
  bool _mightHaveLatentChildren;

  // mapped raster overlay
  std::vector<RasterMappedTo3DTile> _rasterTiles;

  friend class TilesetContentManager;
  friend class MockTilesetContentManagerTestFixture;

public:
  /**
   * @brief A {@link CesiumUtility::DoublyLinkedList} for tile objects.
   */
  typedef CesiumUtility::DoublyLinkedList<Tile, &Tile::_loadedTilesLinks>
      LoadedLinkedList;
};

} // namespace Cesium3DTilesSelection
