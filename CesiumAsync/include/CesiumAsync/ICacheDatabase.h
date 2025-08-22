#pragma once

#include <CesiumAsync/CacheItem.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/Library.h>

#include <cstddef>
#include <optional>

namespace CesiumAsync {
/**
 * @brief Provides database storage interface to cache completed request.
 */
class CESIUMASYNC_API ICacheDatabase {
public:
  virtual ~ICacheDatabase() noexcept = default;

  /**
   * @brief Gets a cache entry from the database.
   *
   * If an error prevents checking the database for the key, this function,
   * depending on the implementation, may log the error. However, it should
   * return `std::nullopt`. It should not throw an exception.
   *
   * @param key The unique key associated with the cache entry.
   * @return The result of the cache lookup, or `std::nullopt` if the key does
   * not exist in the cache or an error occurred.
   */
  virtual std::optional<CacheItem> getEntry(const std::string& key) const = 0;

  /**
   * @brief Store a cache entry in the database.
   *
   * @param key the unique key associated with the response
   * @param expiryTime the time point that this response should be expired. An
   * expired response will be removed when prunning the database.
   * @param url The URL being cached.
   * @param requestMethod The HTTP method being cached.
   * @param requestHeaders The HTTP request headers being cached.
   * @param statusCode The HTTP response status code being cached.
   * @param responseHeaders The HTTP response headers being cached.
   * @param responseData The HTTP response being cached.
   * @return `true` if the entry was successfully stored, or `false` if it could
   * not be stored due to an error.
   */
  virtual bool storeEntry(
      const std::string& key,
      std::time_t expiryTime,
      const std::string& url,
      const std::string& requestMethod,
      const HttpHeaders& requestHeaders,
      uint16_t statusCode,
      const HttpHeaders& responseHeaders,
      const std::span<const std::byte>& responseData) = 0;

  /**
   * @brief Remove cache entries from the database to satisfy the database
   * invariant condition (.e.g exired response or LRU).
   *
   * @return `true` if the database was successfully pruned, or `false` if it
   * could not be pruned due to an errror.
   */
  virtual bool prune() = 0;

  /**
   * @brief Removes all cache entries from the database.
   *
   * @return `true` if the database was successfully cleared, or `false` if it
   * could not be pruned due to an errror.
   */
  virtual bool clearAll() = 0;
};
} // namespace CesiumAsync
