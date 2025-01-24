#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace CesiumAsync {
class IAssetRequest;
}

namespace CesiumIonClient {

/**
 * @brief A response from Cesium ion.
 *
 * @tparam T The type of the response object.
 */
template <typename T> struct Response final {
  /**
   * @brief Creates a new empty `Response`.
   */
  Response();

  /**
   * @brief Creates a `Response` from a completed request and a response value.
   *
   * @param pRequest A completed request. The constructor will attempt to obtain
   * the `httpStatusCode`, `previousPageUrl`, and `nextPageUrl` from this
   * request.
   * @param value The response value.
   */
  Response(
      const std::shared_ptr<CesiumAsync::IAssetRequest>& pRequest,
      T&& value);

  /**
   * @brief Creates a `Response` from a response value, status code, and error
   * information
   *
   * @param value The response value.
   * @param httpStatusCode The HTTP status code of the response.
   * @param errorCode The error code. See \ref Response::errorCode. If no error
   * occurred, pass an empty string.
   * @param errorMessage The error message. See \ref Response::errorMessage. If
   * no error occurred, pass an empty string.
   */
  Response(
      T&& value,
      uint16_t httpStatusCode,
      const std::string& errorCode,
      const std::string& errorMessage);

  /**
   * @brief Creates a `Response` with no value, a status code, and error
   * information.
   *
   * @param httpStatusCode The HTTP status code of the response.
   * @param errorCode The error code. See \ref Response::errorCode.
   * @param errorMessage The error message. See \ref Response::errorMessage.
   */
  Response(
      uint16_t httpStatusCode,
      const std::string& errorCode,
      const std::string& errorMessage);

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

  /**
   * @brief The URL to use to obtain the next page of results, if there is a
   * next page.
   *
   * Call {@link Connection::nextPage} rather than using this field directly.
   */
  std::optional<std::string> nextPageUrl;

  /**
   * @brief The URL to use to obtain the previous page of results, if there is
   * one.
   *
   * Call {@link Connection::previousPage} rather than using this field directly.
   */
  std::optional<std::string> previousPageUrl;
};

/**
 * @brief A non-value, for use with a valueless {@link Response}.
 */
struct NoValue {};

} // namespace CesiumIonClient
