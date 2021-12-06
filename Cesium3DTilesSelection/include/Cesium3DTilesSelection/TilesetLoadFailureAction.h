#pragma once

#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief Specifies the action that should be taken after a tileset load fails.
 */
struct TilesetLoadFailureAction {
  /**
   * @brief Retry the load with the same URL and request headers.
   */
  static TilesetLoadFailureAction retry();

  /**
   * @brief Retry the load with a new URL and the same request headers.
   *
   * @param url The new URL.
   */
  static TilesetLoadFailureAction retryWith(const std::string& url);

  /**
   * @brief Retry the load with a new URL and new request headers.
   *
   * @param url The new URL.
   * @param headers The new headers.
   */
  static TilesetLoadFailureAction
  retryWith(const std::string& url, const std::vector<THeader>& headers);

  /**
   * @brief Give up on this load and consider it failed.
   */
  static TilesetLoadFailureAction giveUp();

private:
  bool isRetry;
  std::optional<std::string> newUrl;
  std::optional<std::vector<THeader>> newHeaders;
};

}; // namespace Cesium3DTilesSelection
