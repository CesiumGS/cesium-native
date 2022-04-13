#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/Exp_ErrorList.h>

#include <CesiumGeometry/Axis.h>

#include <memory>

namespace Cesium3DTilesSelection {
struct TilesetContentLoaderResult {
  std::unique_ptr<TilesetContentLoader> pLoader;

  std::unique_ptr<Tile> pRootTile;

  CesiumGeometry::Axis gltfUpAxis{CesiumGeometry::Axis::Y};

  ErrorList errors;
};
} // namespace Cesium3DTilesSelection
