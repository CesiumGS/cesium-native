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
  gsl::span<const std::byte> view;
};

using AvailabilityView =
    std::variant<SubtreeConstantAvailability, SubtreeBufferViewAvailability>;

class SubtreeAvailability {
public:
  static std::optional<SubtreeAvailability>
  fromSubtree(uint32_t powerOfTwo, Cesium3DTiles::Subtree&& subtree) noexcept;

  static CesiumAsync::Future<std::optional<SubtreeAvailability>> loadSubtree(
      uint32_t powerOf2,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& subtreeUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

  SubtreeAvailability(
      uint32_t powerOf2,
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

  bool isSubtreeAvailable(uint64_t relativeSubtreeMortonId) const noexcept;

private:
  bool isAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      const AvailabilityView& availabilityView) const noexcept;

  bool isAvailableUsingBufferView(
      uint64_t numOfTilesFromRootToParentLevel,
      uint64_t relativeTileMortonId,
      const AvailabilityView& availabilityView) const noexcept;

  Cesium3DTiles::Subtree _subtree;
  uint32_t _childCount;
  uint32_t _powerOf2;
  AvailabilityView _tileAvailability;
  AvailabilityView _subtreeAvailability;
  std::vector<AvailabilityView> _contentAvailability;
};
} // namespace Cesium3DTilesContent
