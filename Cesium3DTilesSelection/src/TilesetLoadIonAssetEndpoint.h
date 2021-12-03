#pragma once

#include "Cesium3DTilesSelection/Tileset.h"

#include <CesiumAsync/Future.h>

#include <spdlog/fwd.h>

namespace CesiumAsync {
class IAssetRequest;
}

namespace Cesium3DTilesSelection {

/**
 * @brief Loads an asset's endpoint information from Cesium ion.
 */
class Tileset::LoadIonAssetEndpoint {
public:
  static CesiumAsync::Future<void> start(Tileset& tileset);

private:
  static CesiumAsync::Future<void> mainThreadHandleResponse(
      Tileset& tileset,
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest);

  static FailedTileAction
  Tileset::LoadIonAssetEndpoint::onIonTileFailed(Tile& failedTile);

  static void mainThreadHandleTokenRefreshResponse(
      Tileset& tileset,
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pIonRequest,
      TileContext* pContext,
      const std::shared_ptr<spdlog::logger>& pLogger);
};

}; // namespace Cesium3DTilesSelection
