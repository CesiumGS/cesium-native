#pragma once

#include "BoundingVolume.h"
#include "Library.h"
#include "TileID.h"
#include "TileRefine.h"
#include "TileSelectionState.h"

#include <Cesium3DTilesSelection/Exp_TileContent.h>
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
   * @brief Default constructor for an empty, uninitialized tile.
   */
  Tile() noexcept;

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
  gsl::span<Tile> getChildren() noexcept {
    return gsl::span<Tile>(this->_children);
  }

  /** @copydoc Tile::getChildren() */
  gsl::span<const Tile> getChildren() const noexcept {
    return gsl::span<const Tile>(this->_children);
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

  void setContent(std::unique_ptr<TileContent>&& pContent) noexcept {
    _pContent = std::move(pContent);
  }

  const TileContent* getContent() const noexcept { return _pContent.get(); }

  TileContent* getContent() noexcept { return _pContent.get(); }

  bool isRenderable() const noexcept;

  bool isRenderContent() const noexcept;

  bool isExternalContent() const noexcept;

  bool isEmptyContent() const noexcept;

  TileLoadState getState() const noexcept;

private:
  void setParent(Tile* pParent) noexcept { this->_pParent = pParent; }

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
  std::unique_ptr<TileContent> _pContent;

public:
  /**
   * @brief A {@link CesiumUtility::DoublyLinkedList} for tile objects.
   */
  typedef CesiumUtility::DoublyLinkedList<Tile, &Tile::_loadedTilesLinks>
      LoadedLinkedList;
};

} // namespace Cesium3DTilesSelection
