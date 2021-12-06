#pragma once

#include "Cesium3DTilesSelection/Tileset.h"

#include <CesiumAsync/Future.h>

#include <string>
#include <vector>

namespace CesiumAsync {
class IAssetRequest;
}

namespace Cesium3DTilesSelection {

class TileContext;

/**
 * @brief Loads a tileset's implicitly-tiled subtree.
 */
class Tileset::LoadSubtree {
public:
  static CesiumAsync::Future<void>
  start(Tileset& tileset, const SubtreeLoadRecord& loadRecord);

private:
  struct Private;
};

}; // namespace Cesium3DTilesSelection
