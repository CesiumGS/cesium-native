#pragma once

#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <optional>

namespace Cesium3DTilesSelection {
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
  SubtreeAvailability(
      uint32_t childCount,
      AvailabilityView tileAvailability,
      AvailabilityView subtreeAvailability,
      std::vector<AvailabilityView>&& contentAvailability,
      std::vector<std::vector<std::byte>>&& buffers);

  bool isTileAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId) const noexcept;

  bool isContentAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      uint64_t contentId) const noexcept;

  bool isSubtreeAvailable(
      uint32_t relativeSubtreeLevel,
      uint64_t relativeSubtreeMortonId) const noexcept;

  static CesiumAsync::Future<std::optional<SubtreeAvailability>> loadSubtree(
      uint32_t childCount,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& subtreeUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

private:
  bool isAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId,
      const AvailabilityView& availabilityView) const noexcept;

  uint32_t _childCount;
  AvailabilityView _tileAvailability;
  AvailabilityView _subtreeAvailability;
  std::vector<AvailabilityView> _contentAvailability;
  std::vector<std::vector<std::byte>> _buffers;
};
} // namespace Cesium3DTilesSelection
