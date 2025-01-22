#pragma once

#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/Library.h>

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <map>
#include <span>
#include <vector>

namespace CesiumAsync {

/**
 * @brief Cache response retrieved from the cache database.
 */
class CESIUMASYNC_API CacheResponse {
public:
  /**
   * @brief Constructor.
   * @param cacheStatusCode the status code of the response
   * @param cacheHeaders the headers of the response
   * @param cacheData the body of the response
   */
  CacheResponse(
      uint16_t cacheStatusCode,
      HttpHeaders&& cacheHeaders,
      std::vector<std::byte>&& cacheData)
      : statusCode(cacheStatusCode),
        headers(std::move(cacheHeaders)),
        data(std::move(cacheData)) {}

  /**
   * @brief The status code of the response.
   */
  uint16_t statusCode;

  /**
   * @brief The headers of the response.
   */
  HttpHeaders headers;

  /**
   * @brief The body data of the response.
   */
  std::vector<std::byte> data;
};

/**
 * @brief Cache request retrieved from the cache database.
 */
class CESIUMASYNC_API CacheRequest {
public:
  /**
   * @brief Constructor.
   * @param cacheHeaders the headers of the request
   * @param cacheMethod the method of the request
   * @param cacheUrl the url of the request
   */
  CacheRequest(
      HttpHeaders&& cacheHeaders,
      std::string&& cacheMethod,
      std::string&& cacheUrl)
      : headers(std::move(cacheHeaders)),
        method(std::move(cacheMethod)),
        url(std::move(cacheUrl)) {}

  /**
   * @brief The headers of the request.
   */
  HttpHeaders headers;

  /**
   * @brief The method of the request.
   */
  std::string method;

  /**
   * @brief The url of the request.
   */
  std::string url;
};

/**
 * @brief Cache item retrieved from the cache database.
 */
class CESIUMASYNC_API CacheItem {
public:
  /**
   * @brief Constructor.
   * @param cacheExpiryTime the time point this cache item will be expired
   * @param request the cache request owned by this item
   * @param response the cache response owned by this item
   */
  CacheItem(
      std::time_t cacheExpiryTime,
      CacheRequest&& request,
      CacheResponse&& response)
      : expiryTime(cacheExpiryTime),
        cacheRequest(std::move(request)),
        cacheResponse(std::move(response)) {}

  /**
   * @brief The time point that this cache item is expired.
   */
  std::time_t expiryTime;

  /**
   * @brief The cache request owned by this cache item.
   */
  CacheRequest cacheRequest;

  /**
   * @brief The cache response owned by this cache item.
   */
  CacheResponse cacheResponse;
};
} // namespace CesiumAsync
