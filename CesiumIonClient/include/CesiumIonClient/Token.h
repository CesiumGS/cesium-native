#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumIonClient {
/**
 * @brief A Cesium ion access token.
 */
struct Token {
  /**
   * @brief The identifier of the token.
   */
  std::string id{};

  /**
   * @brief The name of the token.
   */
  std::string name{};

  /**
   * @brief The token value.
   */
  std::string token{};

  /**
   * @brief The date and time that this token was created, in RFC 3339 format.
   */
  std::string dateAdded{};

  /**
   * @brief The date and time that this token was last modified, in RFC 3339
   * format.
   */
  std::string dateModified{};

  /**
   * @brief The date and time that this token was last used, in RFC 3339 format.
   */
  std::string dateLastUsed{};

  /**
   * @brief The IDs of the assets that can be accessed using this token.
   *
   * If `std::nullopt`, the token allows access to all assets.
   */
  std::optional<std::vector<int64_t>> assetIds{};

  /**
   * @brief True if this is the default token.
   */
  bool isDefault{};

  /**
   * @brief The URLs from which this token can be accessed.
   *
   * If `std::nullopt`, the token can be accessed from any URL.
   */
  std::optional<std::vector<std::string>> allowedUrls{};

  /**
   * @brief The scopes granted by this token.
   */
  std::vector<std::string> scopes{};
};
} // namespace CesiumIonClient
