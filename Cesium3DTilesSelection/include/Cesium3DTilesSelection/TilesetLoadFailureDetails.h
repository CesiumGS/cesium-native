#pragma once

#include <memory>
#include <string>

namespace CesiumAsync {
class IAssetRequest;
}

namespace Cesium3DTilesSelection {

class Tileset;

/**
 * @brief The type of load that failed in `TilesetLoadFailureDetails`.
 */
enum class TilesetLoadType {
  /**
   * @brief An unknown load error.
   */
  Unknown,

  /**
   * @brief A Cesium ion asset endpoint.
   */
  CesiumIon,

  /**
   * @brief The root tileset.json.
   */
  TilesetJson
};

/**
 * Information on a tileset that failed to load.
 */
class TilesetLoadFailureDetails {
public:
  /**
   * @brief The tileset that encountered the load failure.
   */
  const Tileset* pTileset = nullptr;

  /**
   * @brief The type of request that failed to load.
   */
  TilesetLoadType type = TilesetLoadType::Unknown;

  /**
   * The status code of the HTTP response.
   */
  uint16_t statusCode{200};

  /**
   * @brief A human-readable explanation of what failed.
   */
  std::string message = "";
};

} // namespace Cesium3DTilesSelection
