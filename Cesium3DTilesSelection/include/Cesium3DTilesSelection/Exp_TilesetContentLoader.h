#pragma once

#include <Cesium3DTilesSelection/Exp_TileContent.h>
#include <Cesium3DTilesSelection/Exp_TileContentLoadInfo.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <spdlog/logger.h>

#include <memory>

namespace Cesium3DTilesSelection {
class Tile;

class TilesetContentLoader {
public:
  virtual ~TilesetContentLoader() = default;

  void loadTileContent(
      Tile& tile,
      CesiumAsync::AsyncSystem& asyncSystem,
      std::shared_ptr<CesiumAsync::IAssetAccessor>& assetAccessor,
      std::shared_ptr<spdlog::logger>& logger);

  void updateTileContent(Tile& tile);

  bool unloadTileContent(Tile& tile);

private:
  virtual CesiumAsync::Future<TileContentKind>
  doLoadTileContent(const TileContentLoadInfo& loadInfo) = 0;

  virtual void doProcessLoadedContent(Tile& tile) = 0;

  virtual void doUpdateTileContent(Tile& tile) = 0;

  virtual bool doUnloadTileContent(Tile& tile) = 0;
};
} // namespace Cesium3DTilesSelection
