#pragma once

#include "TilesetContentLoader.h"
#include "TilesetContentLoaderResult.h"

#include <Cesium3DTilesSelection/CreditSystem.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeRectangleAvailability.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Projection.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief A loader for terrain described by a layer.json file, and with tiles in
 * quantized-mesh format.
 */
class LayerJsonTerrainLoader : public TilesetContentLoader {
  enum class AvailableState { Available, NotAvailable, Unknown };

public:
  static CesiumAsync::Future<TilesetContentLoaderResult<LayerJsonTerrainLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      const std::string& layerJsonUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      bool showCreditsOnScreen);

  struct Layer {
    Layer(
        const std::string& baseUrl,
        std::string&& version,
        std::vector<std::string>&& tileTemplateUrls,
        CesiumGeometry::QuadtreeRectangleAvailability&& contentAvailability,
        uint32_t maxZooms,
        int32_t availabilityLevels);

    std::string baseUrl;
    std::string version;
    std::vector<std::string> tileTemplateUrls;
    CesiumGeometry::QuadtreeRectangleAvailability contentAvailability;
    std::vector<std::unordered_set<uint64_t>> loadedSubtrees;
    int32_t availabilityLevels;
  };

  LayerJsonTerrainLoader(
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeospatial::Projection& projection,
      std::vector<Layer>&& layers);

  CesiumAsync::Future<TileLoadResult> loadTileContent(
      Tile& tile,
      const TilesetContentOptions& contentOptions,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
      override;

  bool updateTileContent(Tile& tile) override;

private:
  void createTileChildren(Tile& tile);

  bool
  tileIsAvailableInAnyLayer(const CesiumGeometry::QuadtreeTileID& tileID) const;

  AvailableState tileIsAvailableInLayer(
      const CesiumGeometry::QuadtreeTileID& tileID,
      const Layer& layer) const;

  void createChildTile(
      const Tile& parent,
      std::vector<Tile>& children,
      const CesiumGeometry::QuadtreeTileID& childID,
      bool isAvailable);

  CesiumAsync::Future<TileLoadResult>
  upsampleParentTile(Tile& tile, const CesiumAsync::AsyncSystem& asyncSystem);

  CesiumGeometry::QuadtreeTilingScheme _tilingScheme;
  CesiumGeospatial::Projection _projection;
  std::vector<Layer> _layers;
};

} // namespace Cesium3DTilesSelection
