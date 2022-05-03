#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/Availability.h>
#include <variant>
#include <vector>
#include <unordered_set>

namespace Cesium3DTilesSelection {
class ImplicitQuadtreeLoader : public TilesetContentLoader {
  using ImplicitBoundingVolume = std::variant<
      CesiumGeospatial::BoundingRegion,
      CesiumGeospatial::S2CellBoundingVolume,
      CesiumGeometry::OrientedBoundingBox>;

public:
  template <typename ImplicitBoundingVolumeType>
  ImplicitQuadtreeLoader(
      const std::string& baseUrl,
      const std::string& contentUrlTemplate,
      const std::string& subtreeUrlTemplate,
      uint32_t subtreeLevels,
      uint32_t availableLevels,
      ImplicitBoundingVolumeType&& volume)
      : _baseUrl{baseUrl},
        _contentUrlTemplate{contentUrlTemplate},
        _subtreeUrlTemplate{subtreeUrlTemplate},
        _subtreeLevels{subtreeLevels},
        _availableLevels{availableLevels},
        _boundingVolume{std::forward<ImplicitBoundingVolumeType>(volume)},
        _loadedSubtrees(availableLevels / subtreeLevels)
  {}

  CesiumAsync::Future<TileLoadResult> loadTileContent(
      TilesetContentLoader& currentLoader,
      const TileContentLoadInfo& loadInfo,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
      override;

private:
  std::string _baseUrl;
  std::string _contentUrlTemplate;
  std::string _subtreeUrlTemplate;
  uint32_t _subtreeLevels;
  uint32_t _availableLevels;
  ImplicitBoundingVolume _boundingVolume;
  std::vector<std::unordered_set<uint64_t>> _loadedSubtrees;
};
} // namespace Cesium3DTilesSelection
