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
struct SubtreeConstantAvailability {
  bool constant;
};

struct SubtreeBufferViewAvailability {
  gsl::span<std::byte> view;
};

using AvailabilityView =
    std::variant<SubtreeConstantAvailability, SubtreeBufferViewAvailability>;

class SubtreeAvailability {
public:
  static std::optional<SubtreeAvailability> fromSubtree(
      uint32_t powerOfTwo,
      uint32_t levelsInSubtree,
      Cesium3DTiles::Subtree&& subtree) noexcept;

  /**
   * @brief Creates an empty instance with all tiles initially available, while
   * all content and subtrees are initially unavailable.
   *
   * @param powerOfTwo
   * @param levelsInSubtree
   * @return SubtreeAvailability
   */
  static std::optional<SubtreeAvailability>
  createEmpty(uint32_t powerOfTwo, uint32_t levelsInSubtree) noexcept;

  static CesiumAsync::Future<std::optional<SubtreeAvailability>> loadSubtree(
      uint32_t powerOf2,
      uint32_t levelsInSubtree,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& subtreeUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

  SubtreeAvailability(
      uint32_t powerOf2,
      uint32_t levelsInSubtree,
      AvailabilityView tileAvailability,
      AvailabilityView subtreeAvailability,
      std::vector<AvailabilityView>&& contentAvailability,
      Cesium3DTiles::Subtree&& subtree);

  bool isTileAvailable(
      const CesiumGeometry::QuadtreeTileID& subtreeID,
      const CesiumGeometry::QuadtreeTileID& tileID) const noexcept;

  bool isTileAvailable(
      const CesiumGeometry::OctreeTileID& subtreeID,
      const CesiumGeometry::OctreeTileID& tileID) const noexcept;

  bool isTileAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId) const noexcept;

  void setTileAvailable(
      const CesiumGeometry::QuadtreeTileID& subtreeID,
      const CesiumGeometry::QuadtreeTileID& tileID,
      bool isAvailable) noexcept;

  void setTileAvailable(
      const CesiumGeometry::OctreeTileID& subtreeID,
      const CesiumGeometry::OctreeTileID& tileID,
      bool isAvailable) noexcept;

  void setTileAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      bool isAvailable) noexcept;

  bool isContentAvailable(
      const CesiumGeometry::QuadtreeTileID& subtreeID,
      const CesiumGeometry::QuadtreeTileID& tileID,
      uint64_t contentId) const noexcept;

  bool isContentAvailable(
      const CesiumGeometry::OctreeTileID& subtreeID,
      const CesiumGeometry::OctreeTileID& tileID,
      uint64_t contentId) const noexcept;

  bool isContentAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      uint64_t contentId) const noexcept;

  void setContentAvailable(
      const CesiumGeometry::QuadtreeTileID& subtreeID,
      const CesiumGeometry::QuadtreeTileID& tileID,
      uint64_t contentId,
      bool isAvailable) noexcept;

  void setContentAvailable(
      const CesiumGeometry::OctreeTileID& subtreeID,
      const CesiumGeometry::OctreeTileID& tileID,
      uint64_t contentId,
      bool isAvailable) noexcept;

  void setContentAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      uint64_t contentId,
      bool isAvailable) noexcept;

  bool isSubtreeAvailable(uint64_t relativeSubtreeMortonId) const noexcept;

  bool isSubtreeAvailable(
      const CesiumGeometry::QuadtreeTileID& thisSubtreeID,
      const CesiumGeometry::QuadtreeTileID& checkSubtreeID) const noexcept;

  bool isSubtreeAvailable(
      const CesiumGeometry::OctreeTileID& thisSubtreeID,
      const CesiumGeometry::OctreeTileID& checkSubtreeID) const noexcept;

  void setSubtreeAvailable(
      uint64_t relativeSubtreeMortonId,
      bool isAvailable) noexcept;

  void setSubtreeAvailable(
      const CesiumGeometry::QuadtreeTileID& thisSubtreeID,
      const CesiumGeometry::QuadtreeTileID& setSubtreeID,
      bool isAvailable) noexcept;

  void setSubtreeAvailable(
      const CesiumGeometry::OctreeTileID& thisSubtreeID,
      const CesiumGeometry::OctreeTileID& setSubtreeID,
      bool isAvailable) noexcept;

  Cesium3DTiles::Subtree& getSubtree() noexcept { return this->_subtree; }
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
