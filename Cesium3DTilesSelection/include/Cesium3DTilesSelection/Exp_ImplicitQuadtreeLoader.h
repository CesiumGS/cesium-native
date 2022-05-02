#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <variant>

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
      uint32_t maximumLevel,
      ImplicitBoundingVolumeType&& volume)
      : _baseUrl{baseUrl},
        _contentUrlTemplate{contentUrlTemplate},
        _subtreeUrlTemplate{subtreeUrlTemplate},
        _boundingVolume{std::forward<ImplicitBoundingVolumeType>(volume)} {
    (void)(subtreeLevels);
    (void)(maximumLevel);
  }

  CesiumAsync::Future<TileLoadResult> loadTileContent(
      TilesetContentLoader& currentLoader,
      const TileContentLoadInfo& loadInfo,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
      override;

private:
  std::string _baseUrl;
  std::string _contentUrlTemplate;
  std::string _subtreeUrlTemplate;
  ImplicitBoundingVolume _boundingVolume;
};
} // namespace Cesium3DTilesSelection
