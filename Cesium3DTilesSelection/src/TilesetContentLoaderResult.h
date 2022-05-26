#pragma once

#include "TilesetContentLoader.h"

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

struct TilesetContentLoaderResult {
  std::unique_ptr<TilesetContentLoader> pLoader;

  std::unique_ptr<Tile> pRootTile;

  CesiumGeometry::Axis gltfUpAxis{CesiumGeometry::Axis::Y};

  std::vector<LoaderCreditResult> credits;

  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;

  ErrorList errors;
};
} // namespace Cesium3DTilesSelection
