#pragma once

#include <memory>
#include <string>

namespace CesiumAsync {
class IAssetRequest;
}

namespace Cesium3DTilesSelection {

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
  TileSubtree
};

struct TilesetLoadFailureDetails {
  TilesetLoadFailureDetails();
  TilesetLoadFailureDetails(const std::exception& exception);
  TilesetLoadFailureDetails(
      TilesetLoadType type,
      std::shared_ptr<CesiumAsync::IAssetRequest> pRequest,
      const std::string& message);

  /**
   * @brief The type of request that failed to load.
   */
  TilesetLoadType type;

  /**
   * @brief The request that failed.
   */
  std::shared_ptr<CesiumAsync::IAssetRequest> pRequest;

  /**
   * @brief A human-readable explanation of what failed.
   */
  std::string message;
};

} // namespace Cesium3DTilesSelection
