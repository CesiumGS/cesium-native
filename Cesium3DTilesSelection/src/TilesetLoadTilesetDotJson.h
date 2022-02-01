#pragma once

#include "Cesium3DTilesSelection/Tileset.h"

#include <CesiumAsync/Future.h>

#include <string>
#include <utility>
#include <vector>

namespace Cesium3DTilesSelection {

class TileContext;

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
  struct Private;
};

} // namespace Cesium3DTilesSelection
