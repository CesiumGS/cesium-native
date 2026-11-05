#pragma once

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <memory>
#include <vector>

namespace Cesium3DTilesSelection {
template <typename TLoader> struct ExternalContentInitializer {
  // Have to use shared_ptr here to make this functor copyable. Otherwise,
  // std::function won't work with move-only type as it's a type-erasured
  // container. Unfortunately, std::move_only_function is scheduled for C++23.
  std::shared_ptr<TilesetContentLoaderResult<TLoader>> pExternalTilesetLoaders;
  TLoader* pTilesetLoader;
  TileExternalContent externalContent;

  void operator()(Tile& tile) {
    TileExternalContent* pExternalContent =
        tile.getContent().getExternalContent();
    if (pExternalContent) {
      *pExternalContent = std::move(externalContent);
      std::unique_ptr<Tile>& pExternalRoot = pExternalTilesetLoaders->pRootTile;
      if (pExternalRoot) {
        // propagate all the external tiles to be the children of this tile
        std::vector<Tile> children;
        children.emplace_back(std::move(*pExternalRoot));
        tile.createChildTiles(std::move(children));

        // save the loader of the external tileset in this loader
        pTilesetLoader->addChildLoader(
            std::move(pExternalTilesetLoaders->pLoader));
      }
    }
  }
};
} // namespace Cesium3DTilesSelection
