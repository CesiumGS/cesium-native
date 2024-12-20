#pragma once

#include <CesiumIonClient/Token.h>

namespace CesiumIonClient {

/**
 * @brief A list of Cesium ion access tokens, as returned by the "List Tokens"
 * service.
 */
struct TokenList {
  /**
   * @brief The tokens.
   *
   */
  std::vector<Token> items;
};

} // namespace CesiumIonClient
