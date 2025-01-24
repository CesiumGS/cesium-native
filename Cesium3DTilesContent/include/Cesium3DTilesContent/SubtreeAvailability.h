#pragma once

#include <Cesium3DTiles/Subtree.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <optional>

namespace CesiumGeometry {
struct QuadtreeTileID;
struct OctreeTileID;
} // namespace CesiumGeometry

namespace Cesium3DTiles {
struct ImplicitTiling;
} // namespace Cesium3DTiles

namespace Cesium3DTilesContent {

/**
 * @brief Indicates how an implicit tile is subdivided.
 */
enum class ImplicitTileSubdivisionScheme {
  /**
   * @brief Implicit tiles are divided into four children, forming a quadree.
   */
  Quadtree,

  /**
   * @brief Implicit tiles are divided into eight children, forming an octree.
   */
  Octree
};

/**
 * @brief Supports querying and modifying the various types of availablity
 * information included in a {@link Cesium3DTiles::Subtree}.
 */
class SubtreeAvailability {
public:
  /**
   * @brief Creates an instance from a `Subtree`.
   *
   * @param subdivisionScheme The subdivision scheme of the subtree (quadtree or
   * octree).
   * @param levelsInSubtree The number of levels in this subtree.
   * @param subtree The subtree.
   * @return The subtree availability, or std::nullopt if the subtree definition
   * is invalid.
   */
  static std::optional<SubtreeAvailability> fromSubtree(
      ImplicitTileSubdivisionScheme subdivisionScheme,
      uint32_t levelsInSubtree,
      Cesium3DTiles::Subtree&& subtree) noexcept;

  /**
   * @brief Creates an empty instance with all tiles initially available, while
   * all content and subtrees are initially unavailable.
   *
   * @param subdivisionScheme The subdivision scheme of the subtree (quadtree or
   * octree).
   * @param levelsInSubtree The number of levels in this subtree.
   * @return The subtree availability, or std::nullopt if the subtree definition
   * is invalid.
   */
  static std::optional<SubtreeAvailability> createEmpty(
      ImplicitTileSubdivisionScheme subdivisionScheme,
      uint32_t levelsInSubtree) noexcept;

  /**
   * @brief Asynchronously loads a subtree from a URL. The resource downloaded
   * from the URL may be either a JSON or a binary subtree file.
   *
   * @param subdivisionScheme The subdivision scheme of the subtree (quadtree or
   * octree).
   * @param levelsInSubtree The number of levels in this subtree.
   * @param asyncSystem The async system with which to do background work.
   * @param pAssetAccessor The asset accessor to use to retrieve the subtree
   * resource from the URL.
   * @param pLogger The logger to which to load errors and warnings that occur
   * during subtree load.
   * @param subtreeUrl The URL from which to retrieve the subtree file.
   * @param requestHeaders HTTP headers to include in the request for the
   * subtree file.
   * @return A future that resolves to a `SubtreeAvailability` instance for the
   * subtree file, or std::nullopt if something goes wrong.
   */
  static CesiumAsync::Future<std::optional<SubtreeAvailability>> loadSubtree(
      ImplicitTileSubdivisionScheme subdivisionScheme,
      uint32_t levelsInSubtree,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& subtreeUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

  /**
   * @brief An AvailibilityView that indicates that either all tiles are
   * available or all tiles are unavailable.
   */
  struct SubtreeConstantAvailability {
    /**
     * @brief True if all tiles are availabile, false if all tiles are
     * unavailable.
     */
    bool constant;
  };

  /**
   * @brief An AvailabilityView that accesses availability information from a
   * bitstream.
   */
  struct SubtreeBufferViewAvailability {
    /**
     * @brief The buffer from which to read and write availability information.
     */
    std::span<std::byte> view;
  };

  /**
   * @brief A mechanism for accessing availability information. It may be a
   * constant value, or it may be read from a bitstream.
   */
  using AvailabilityView =
      std::variant<SubtreeConstantAvailability, SubtreeBufferViewAvailability>;

  /**
   * @brief Constructs a new instance.
   *
   * @param subdivisionScheme The subdivision scheme of the subtree (quadtree or
   * octree).
   * @param levelsInSubtree The number of levels in this subtree.
   * @param tileAvailability A view on the tile availability. If backed by a
   * buffer, the buffer is expected to be in `subtree`.
   * @param subtreeAvailability A view on the subtree availability. If backed by
   * a buffer, the buffer is expected to be in `subtree`.
   * @param contentAvailability A view on the content availability. If backed by
   * a buffer, the buffer is expected to be in `subtree`.
   * @param subtree The subtree with which this instance queries and modifies
   * availability information.
   */
  SubtreeAvailability(
      ImplicitTileSubdivisionScheme subdivisionScheme,
      uint32_t levelsInSubtree,
      AvailabilityView tileAvailability,
      AvailabilityView subtreeAvailability,
      std::vector<AvailabilityView>&& contentAvailability,
      Cesium3DTiles::Subtree&& subtree);

  /**
   * @brief Determines if a given tile in the quadtree is available.
   *
   * @param subtreeId The ID of the root tile of the subtree.
   * @param tileId The ID of the tile to query.
   * @return True if the tile is available; otherwise, false.
   */
  bool isTileAvailable(
      const CesiumGeometry::QuadtreeTileID& subtreeId,
      const CesiumGeometry::QuadtreeTileID& tileId) const noexcept;

  /**
   * @brief Determines if a given tile in the octree is available.
   *
   * @param subtreeId The ID of the root tile of the subtree.
   * @param tileId The ID of the tile to query.
   * @return True if the tile is available; otherwise, false.
   */
  bool isTileAvailable(
      const CesiumGeometry::OctreeTileID& subtreeId,
      const CesiumGeometry::OctreeTileID& tileId) const noexcept;

  /**
   * @brief Determines if a given tile in the subtree is available.
   *
   * @param relativeTileLevel The level of the tile to query, relative to the
   * root of the subtree.
   * @param relativeTileMortonId The Morton ID of the tile to query. See
   * {@link ImplicitTilingUtilities::computeRelativeMortonIndex}.
   * @return True if the tile is available; otherwise, false.
   */
  bool isTileAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId) const noexcept;

  /**
   * @brief Sets the availability state of a given tile in the quadtree.
   *
   * @param subtreeId The ID of the root tile of the subtree.
   * @param tileId The ID of the tile for which to set availability.
   * @param isAvailable The new availability state for the tile.
   */
  void setTileAvailable(
      const CesiumGeometry::QuadtreeTileID& subtreeId,
      const CesiumGeometry::QuadtreeTileID& tileId,
      bool isAvailable) noexcept;

  /**
   * @brief Sets the availability state of a given tile in the octree.
   *
   * @param subtreeId The ID of the root tile of the subtree.
   * @param tileId The ID of the tile for which to set availability.
   * @param isAvailable The new availability state for the tile.
   */
  void setTileAvailable(
      const CesiumGeometry::OctreeTileID& subtreeId,
      const CesiumGeometry::OctreeTileID& tileId,
      bool isAvailable) noexcept;

  /**
   * @brief Sets the availability state of a given tile in the subtree.
   *
   * @param relativeTileLevel The level of the tile for which to set
   * availability, relative to the root of the subtree.
   * @param relativeTileMortonId The Morton ID of the tile for which to set
   * availability. See
   * {@link ImplicitTilingUtilities::computeRelativeMortonIndex}.
   * @param isAvailable The new availability state of the tile.
   */
  void setTileAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      bool isAvailable) noexcept;

  /**
   * @brief Determines if content for a given tile in the quadtree is available.
   *
   * @param subtreeId The ID of the root tile of the subtree.
   * @param tileId The ID of the tile to query.
   * @param contentId The ID of the content to query.
   * @return True if the tile's content is available; otherwise, false.
   */
  bool isContentAvailable(
      const CesiumGeometry::QuadtreeTileID& subtreeId,
      const CesiumGeometry::QuadtreeTileID& tileId,
      uint64_t contentId) const noexcept;

  /**
   * @brief Determines if content for a given tile in the octree is available.
   *
   * @param subtreeId The ID of the root tile of the subtree.
   * @param tileId The ID of the tile to query.
   * @param contentId The ID of the content to query.
   * @return True if the tile's content is available; otherwise, false.
   */
  bool isContentAvailable(
      const CesiumGeometry::OctreeTileID& subtreeId,
      const CesiumGeometry::OctreeTileID& tileId,
      uint64_t contentId) const noexcept;

  /**
   * @brief Determines if content for a given tile in the subtree is available.
   *
   * @param relativeTileLevel The level of the tile to query, relative to the
   * root of the subtree.
   * @param relativeTileMortonId The Morton ID of the tile to query. See
   * {@link ImplicitTilingUtilities::computeRelativeMortonIndex}.
   * @param contentId The ID of the content to query.
   * @return True if the tile's content is available; otherwise, false.
   */
  bool isContentAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      uint64_t contentId) const noexcept;

  /**
   * @brief Sets the availability state of the content for a given tile in the
   * quadtree.
   *
   * @param subtreeId The ID of the root tile of the subtree.
   * @param tileId The ID of the tile for which to set content availability.
   * @param contentId The ID of the content to query.
   * @param isAvailable The new availability state for the tile's content.
   */
  void setContentAvailable(
      const CesiumGeometry::QuadtreeTileID& subtreeId,
      const CesiumGeometry::QuadtreeTileID& tileId,
      uint64_t contentId,
      bool isAvailable) noexcept;

  /**
   * @brief Sets the availability state of the content for a given tile in the
   * octree.
   *
   * @param subtreeId The ID of the root tile of the subtree.
   * @param tileId The ID of the tile for which to set content availability.
   * @param contentId The ID of the content to query.
   * @param isAvailable The new availability state for the tile's content.
   */
  void setContentAvailable(
      const CesiumGeometry::OctreeTileID& subtreeId,
      const CesiumGeometry::OctreeTileID& tileId,
      uint64_t contentId,
      bool isAvailable) noexcept;

  /**
   * @brief Sets the availability state of the content for a given tile in the
   * subtree.
   *
   * @param relativeTileLevel The level of the tile for which to set
   * content availability, relative to the root of the subtree.
   * @param relativeTileMortonId The Morton ID of the tile for which to set
   * content availability. See
   * {@link ImplicitTilingUtilities::computeRelativeMortonIndex}.
   * @param contentId The ID of the content to query.
   * @param isAvailable The new availability state for the tile's content.
   */
  void setContentAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      uint64_t contentId,
      bool isAvailable) noexcept;

  /**
   * @brief Determines if the subtree rooted at the given tile is available.
   *
   * The provided `checkSubtreeID` must be a child of the leaves of this
   * subtree.
   *
   * @param thisSubtreeID The ID of the root tile of this subtree.
   * @param checkSubtreeID The ID of the tile to query to see if its subtree is
   * available.
   * @return True if the subtree is available; otherwise, false.
   */
  bool isSubtreeAvailable(
      const CesiumGeometry::QuadtreeTileID& thisSubtreeID,
      const CesiumGeometry::QuadtreeTileID& checkSubtreeID) const noexcept;

  /**
   * @brief Determines if the subtree rooted at the given tile is available.
   *
   * The provided `checkSubtreeID` must be a child of the leaves of this
   * subtree.
   *
   * @param thisSubtreeID The ID of the root tile of this subtree.
   * @param checkSubtreeID The ID of the tile to query to see if its subtree is
   * available.
   * @return True if the subtree is available; otherwise, false.
   */
  bool isSubtreeAvailable(
      const CesiumGeometry::OctreeTileID& thisSubtreeID,
      const CesiumGeometry::OctreeTileID& checkSubtreeID) const noexcept;

  /**
   * @brief Determines if the subtree rooted at the given tile is available.
   *
   * The provided `relativeSubtreeMortonId` must refer to a child of the leaves
   * of this subtree.
   *
   * @param relativeSubtreeMortonId The Morton ID of the tile for which to check
   * subtree availability. See
   * {@link ImplicitTilingUtilities::computeRelativeMortonIndex}.
   * @return True if the subtree is available; otherwise, false.
   */
  bool isSubtreeAvailable(uint64_t relativeSubtreeMortonId) const noexcept;

  /**
   * @brief Sets the availability state of the child quadtree rooted at the
   * given tile.
   *
   * The provided `setSubtreeID` must be a child of the leaves of this
   * subtree.
   *
   * @param thisSubtreeID The ID of the root tile of this subtree.
   * @param setSubtreeID The ID of the tile to query to see if its subtree is
   * available.
   * @param isAvailable The new availability state for the subtree.
   */
  void setSubtreeAvailable(
      const CesiumGeometry::QuadtreeTileID& thisSubtreeID,
      const CesiumGeometry::QuadtreeTileID& setSubtreeID,
      bool isAvailable) noexcept;

  /**
   * @brief Sets the availability state of the child octree rooted at the given
   * tile.
   *
   * The provided `setSubtreeID` must be a child of the leaves of this
   * subtree.
   *
   * @param thisSubtreeID The ID of the root tile of this subtree.
   * @param setSubtreeID The ID of the tile to query to see if its subtree is
   * available.
   * @param isAvailable The new availability state for the subtree.
   */
  void setSubtreeAvailable(
      const CesiumGeometry::OctreeTileID& thisSubtreeID,
      const CesiumGeometry::OctreeTileID& setSubtreeID,
      bool isAvailable) noexcept;

  /**
   * @brief Sets the availability state of the child subtree rooted at the given
   * tile.
   *
   * The provided `relativeSubtreeMortonId` must refer to a child of the leaves
   * of this subtree.
   *
   * @param relativeSubtreeMortonId The Morton ID of the tile for which to set
   * subtree availability. See
   * {@link ImplicitTilingUtilities::computeRelativeMortonIndex}.
   * @param isAvailable The new availability state.
   */
  void setSubtreeAvailable(
      uint64_t relativeSubtreeMortonId,
      bool isAvailable) noexcept;

  /**
   * @brief Gets the subtree that this instance queries and modifies.
   */
  const Cesium3DTiles::Subtree& getSubtree() const noexcept {
    return this->_subtree;
  }

private:
  bool isAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      const AvailabilityView& availabilityView) const noexcept;
  void setAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      AvailabilityView& availabilityView,
      bool isAvailable) noexcept;

  bool isAvailableUsingBufferView(
      uint64_t numOfTilesFromRootToParentLevel,
      uint64_t relativeTileMortonId,
      const AvailabilityView& availabilityView) const noexcept;
  void setAvailableUsingBufferView(
      uint64_t numOfTilesFromRootToParentLevel,
      uint64_t relativeTileMortonId,
      AvailabilityView& availabilityView,
      bool isAvailable) noexcept;

  uint32_t _powerOf2;
  uint32_t _levelsInSubtree;
  Cesium3DTiles::Subtree _subtree;
  uint32_t _childCount;
  AvailabilityView _tileAvailability;
  AvailabilityView _subtreeAvailability;
  std::vector<AvailabilityView> _contentAvailability;
};
} // namespace Cesium3DTilesContent
