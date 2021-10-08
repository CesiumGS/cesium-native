#pragma once

#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileRectangularRange.h>
#include <CesiumGeospatial/GlobeRectangle.h>

#include <rapidjson/document.h>

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

struct TerrainLayerJson {
  std::string attribution;
  std::vector<CesiumGeometry::QuadtreeTileRectangularRange> available;
  CesiumGeospatial::GlobeRectangle bounds;
  std::string description;
  std::vector<std::string> extensions;
  std::string format;
  int32_t maxzoom = 0;
  int32_t metadataAvailability = 0;
  int32_t minzoom = 0;
  std::string name;
  std::string parentUrl;
  std::string projection;
  std::string scheme;
  std::vector<std::string> tiles;
  std::string version;

  std::unique_ptr<TerrainLayerJson> pResolvedParent;

  CesiumAsync::Future<TerrainLayerJson> resolveParents(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& baseUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>&
          requestHeaders) &&;

  static TerrainLayerJson parse(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const gsl::span<const std::byte>& data);

  static TerrainLayerJson parse(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const rapidjson::Value& jsonValue);
};

} // namespace Cesium3DTilesSelection
