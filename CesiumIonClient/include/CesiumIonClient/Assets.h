#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumIonClient {
/**
 * @brief A Cesium ion Asset, such as a 3D Tiles tileset or an imagery layer.
 */
struct Asset {
  /**
   * @brief The unique identifier for this asset.
   */
  int64_t id = -1;

  /**
   * @brief The name of this asset.
   */
  std::string name;

  /**
   * @brief A Markdown compatible string describing this asset.
   */
  std::string description;

  /**
   * @brief A Markdown compatible string containing any required attribution for
   * this asset.
   *
   * Clients will be required to display this attribution to end users.
   */
  std::string attribution;

  /**
   * @brief The asset's [type](https://cesium.com/docs/rest-api/#tag/Assets)./
   */
  std::string type;

  /**
   * @brief The number of bytes this asset occupies in the user's account.
   */
  int64_t bytes = 0;

  /**
   * @brief The date and time that this asset was created in [RFC
   * 3339](https://tools.ietf.org/html/rfc3339) format.
   */
  std::string dateAdded;

  /**
   * @brief Describes the state of the asset during the upload and tiling
   * processes.
   *
   * | *Value* | *Description* |
   * | AWAITING_FILES | The asset has been created but the server is waiting for
   * source data to be uploaded. | | NOT_STARTED | The server was notified that
   * all source data was uploaded and is waiting for the tiling process to
   * start. | | IN_PROGRESS | The source data is being tiled, see
   * percentComplete to monitor progress. | | COMPLETE | The asset was tiled
   * successfully. | | DATA_ERROR | The uploaded source data was invalid or
   * unsupported. | | ERROR | An internal error occurred during the tiling
   * process. Please contact support@cesium.com. |
   */
  std::string status;

  /**
   * @brief The percentage progress of the tiling pipeline preparing this asset.
   */
  int8_t percentComplete = 0;
};

/**
 * @brief A page of assets obtained from the Cesium ion `v1/assets` endpoint,
 * including a link to obtain the next page, if one exists.
 */
struct Assets {
  /**
   * @brief An [RFC 5988](https://tools.ietf.org/html/rfc5988) formatted string
   * with a relation type of next which points to the next page of data on the
   * server, if it exists.
   *
   * This url should be treated as an opaque link as the actual format may
   * change at any time.
   */
  std::string link;

  /**
   * @brief A page of assets.
   */
  std::vector<Asset> items;
};
} // namespace CesiumIonClient
