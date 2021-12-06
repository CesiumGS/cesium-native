#pragma once

#include "Cesium3DTilesSelection/Tileset.h"

#include <CesiumAsync/Future.h>

#include <glm/fwd.hpp>
#include <rapidjson/fwd.h>
#include <spdlog/fwd.h>

namespace Cesium3DTilesSelection {

class Tile;
class TileContext;

/**
 * @brief Loads a tile (and its children) from the JSON representation of the
 * tile as expressed in the 3D Tiles tileset.json.
 */
class Tileset::LoadTileFromJson {
public:
  static void execute(
      Tile& tile,
      std::vector<std::unique_ptr<TileContext>>& newContexts,
      const rapidjson::Value& tileJson,
      const glm::dmat4& parentTransform,
      TileRefine parentRefine,
      const TileContext& context,
      const std::shared_ptr<spdlog::logger>& pLogger);
};

}; // namespace Cesium3DTilesSelection
