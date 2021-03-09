#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace CesiumIonClient {
/**
 * @brief A response from Cesium ion.
 *
 * @tparam T The type of the response object.
 */
template <typename T> struct Response final {
  /**
   * @brief The response value, or `std::nullopt` if the response was
   * unsuccessful.
   */
  std::optional<T> value;

  /**
   * @brief The HTTP status code returned by Cesium ion.
   */
  uint16_t httpStatusCode;

  /**
   * @brief The error code, or empty string if there was no error.
   *
   * If no response is received at all, the code will be `"NoResponse"`.
   *
   * If Cesium ion returns an error, this will be the `code` reported by Cesium
   * ion.
   *
   * If Cesium ion reports success but an error occurs while attempting to parse
   * the response, the code will be
   * `"ParseError"`.
   */
  std::string errorCode;

  /**
   * @brief The error message returned, or an empty string if there was no
   * error.
   *
   * If Cesium ion returns an error, this will be the `message` reported by
   * Cesium ion. If Cesium ion reports success but another error occurs, the
   * message will contain further details of the error.
   */
  std::string errorMessage;
};
} // namespace CesiumIonClient
