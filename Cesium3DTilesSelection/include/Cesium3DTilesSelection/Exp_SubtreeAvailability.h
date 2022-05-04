#pragma once

#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/Future.h>
#include <optional>

namespace Cesium3DTilesSelection {
class SubtreeAvailabitity {
public:
  bool
  isTileAvailable(uint32_t tileLevel, uint64_t tileMortonId) const noexcept;

  bool isTileContentAvailable(
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
};
} // namespace Cesium3DTilesSelection
