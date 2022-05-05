#pragma once

#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/Future.h>
#include <optional>

namespace Cesium3DTilesSelection {
struct SubtreeConstantAvailability {
  bool constant;
};

struct SubtreeBufferViewAvailability {
  gsl::span<const std::byte> view;
};

using AvailabilityView = std::variant<SubtreeConstantAvailability, SubtreeBufferViewAvailability>;

class SubtreeAvailabitity {
public:
  SubtreeAvailabitity(
      AvailabilityView tileAvailability,
      AvailabilityView subtreeAvailability,
      std::vector<AvailabilityView>&& contentAvailability,
      std::vector<std::vector<std::byte>>&& buffers);

  bool
  isTileAvailable(uint32_t tileLevel, uint64_t tileMortonId) const noexcept;

  bool isContentAvailable(
      uint32_t tileLevel,
      uint64_t tileMortonId,
      uint64_t contentId) const noexcept;

  bool isSubtreeAvailable(uint32_t subtreeLevel, uint64_t subtreeMortonId)
      const noexcept;

  static CesiumAsync::Future<std::optional<SubtreeAvailabitity>> loadSubtree(
      const TilesetExternals& externals,
      const std::string& subtreeUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

private:
  AvailabilityView _tileAvailability;
  AvailabilityView _subtreeAvailability;
  std::vector<AvailabilityView> _contentAvailability;
  std::vector<std::vector<std::byte>> _buffers;
};
} // namespace Cesium3DTilesSelection
