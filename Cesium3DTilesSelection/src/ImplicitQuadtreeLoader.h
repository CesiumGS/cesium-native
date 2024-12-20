#pragma once

#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>

#include <cmath>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Cesium3DTilesSelection {
using ImplicitQuadtreeBoundingVolume = std::variant<
    CesiumGeospatial::BoundingRegion,
    CesiumGeospatial::S2CellBoundingVolume,
    CesiumGeometry::OrientedBoundingBox>;

class ImplicitQuadtreeLoader : public TilesetContentLoader {
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

  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  TileChildrenResult createTileChildren(
      const Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) override;

  uint32_t getSubtreeLevels() const noexcept;

  uint32_t getAvailableLevels() const noexcept;

  const ImplicitQuadtreeBoundingVolume& getBoundingVolume() const noexcept;

  void addSubtreeAvailability(
      const CesiumGeometry::QuadtreeTileID& subtreeID,
      Cesium3DTilesContent::SubtreeAvailability&& subtreeAvailability);

private:
  std::string _baseUrl;
  std::string _contentUrlTemplate;
  std::string _subtreeUrlTemplate;
  uint32_t _subtreeLevels;
  uint32_t _availableLevels;
  ImplicitQuadtreeBoundingVolume _boundingVolume;
  std::vector<
      std::unordered_map<uint64_t, Cesium3DTilesContent::SubtreeAvailability>>
      _loadedSubtrees;
};
} // namespace Cesium3DTilesSelection
