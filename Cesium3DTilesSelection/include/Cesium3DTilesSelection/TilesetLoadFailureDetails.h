#pragma once

#include <memory>
#include <string>

namespace CesiumAsync {
class IAssetRequest;
}

namespace Cesium3DTilesSelection {

class Tileset;
class Tile;

/**
 * @brief The type of load that failed in {@link TilesetLoadFailureDetails}.
 */
enum class TilesetLoadType {
  /**
   * @brief A Cesium ion asset endpoint.
   */
  CesiumIon,

  /**
   * @brief A tileset.json.
   */
  TilesetJson,

  /**
   * @brief The content of a tile.
   */
  TileContent,

  /**
   * @brief An implicit tiling subtree.
   *
   */
  TileSubtree,

  /**
   * @brief An unknown load error.
   */
  Unknown
};

class TilesetLoadFailureDetails {
public:
  /**
   * @brief The tileset that encountered the load failure.
   */
  const Tileset* pTileset = nullptr;

  /**
   * @brief The tile that encountered the load failure, or `nullptr` if the
   * failure was not related to a particular tile.
   */
  const Tile* pTile = nullptr;

  /**
   * @brief The type of request that failed to load.
   */
  TilesetLoadType type = TilesetLoadType::Unknown;

  /**
   * @brief The request that failed. The request itself may have succeeded, but
   * the failure occurred while processing this request.
   */
  std::shared_ptr<CesiumAsync::IAssetRequest> pRequest = nullptr;

  /**
   * @brief A human-readable explanation of what failed.
   */
  std::string message = "";
};

} // namespace Cesium3DTilesSelection
