#pragma once

#include "Cesium3DTilesSelection/Tileset.h"

#include <CesiumAsync/Future.h>

#include <spdlog/fwd.h>

namespace CesiumAsync {
class IAssetRequest;
}

namespace Cesium3DTilesSelection {

/**
 * @brief Loads a tileset's root tileset.json.
 */
class Tileset::LoadTilesetDotJson {
public:
  static CesiumAsync::Future<void> start(
      Tileset& tileset,
      const std::string& url,
      const std::vector<std::pair<std::string, std::string>>& headers = {},
      std::unique_ptr<TileContext>&& pContext = nullptr);

private:
  static LoadResult workerThreadHandleResponse(
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest,
      std::unique_ptr<TileContext>&& pContext,
      const std::shared_ptr<spdlog::logger>& pLogger,
      bool useWaterMask);

  static void loadTerrainTile(
      Tile& tile,
      const rapidjson::Value& layerJson,
      TileContext& context,
      const std::shared_ptr<spdlog::logger>& pLogger,
      bool useWaterMask);
};

}; // namespace Cesium3DTilesSelection
