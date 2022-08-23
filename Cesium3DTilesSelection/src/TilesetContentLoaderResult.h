#pragma once

#include <Cesium3DTilesSelection/ErrorList.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>

#include <memory>
#include <vector>

namespace Cesium3DTilesSelection {
struct LoaderCreditResult {
  std::string creditText;

  bool showOnScreen;
};

template <class TilesetContentLoaderType> struct TilesetContentLoaderResult {
  std::unique_ptr<TilesetContentLoaderType> pLoader;

  std::unique_ptr<Tile> pRootTile;

  std::vector<LoaderCreditResult> credits;

  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;

  ErrorList errors;
};
} // namespace Cesium3DTilesSelection
