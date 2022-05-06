#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/Exp_SubtreeAvailability.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/Availability.h>
#include <cmath>
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
        _loadedSubtrees(static_cast<size_t>(std::ceil(
            static_cast<float>(availableLevels) /
            static_cast<float>(subtreeLevels)))) {}

  CesiumAsync::Future<TileLoadResult> loadTileContent(
      TilesetContentLoader& currentLoader,
      const TileContentLoadInfo& loadInfo,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
      override;

private:
  static std::string resolveUrl(
      const std::string& baseUrl,
      const std::string& urlTemplate,
      const CesiumGeometry::QuadtreeTileID& quadtreeID);

  std::string _baseUrl;
  std::string _contentUrlTemplate;
  std::string _subtreeUrlTemplate;
  uint32_t _subtreeLevels;
  uint32_t _availableLevels;
  ImplicitBoundingVolume _boundingVolume;
  std::vector<std::unordered_map<uint64_t, SubtreeAvailability>>
      _loadedSubtrees;
};
} // namespace Cesium3DTilesSelection
