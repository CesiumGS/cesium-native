#pragma once

#include "IAssetAccessor.h"
#include "IAssetRequest.h"
#include "IAssetResponse.h"
#include "ICacheDatabase.h"
#include "ThreadPool.h"

#include <spdlog/fwd.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

namespace CesiumAsync {
class AsyncSystem;

/**
 * @brief A cached HTTP response.
 */
class CacheAssetResponse : public IAssetResponse {
public:
  CacheAssetResponse(const CacheItem* pCacheItem) noexcept;
  virtual uint16_t statusCode() const noexcept override;
  virtual std::string contentType() const override;
  virtual const HttpHeaders& headers() const noexcept override;
  virtual gsl::span<const std::byte> data() const noexcept override;
  gsl::span<const std::byte> clientData() const noexcept;

private:
  const CacheItem* _pCacheItem;
};

/**
 * @brief A cached HTTP request.
 */
class CacheAssetRequest : public IAssetRequest {
public:
  CacheAssetRequest(CacheItem&& cacheItem);
  virtual const std::string& method() const noexcept override;
  virtual const std::string& url() const noexcept override;
  virtual const HttpHeaders& headers() const noexcept override;
  virtual const IAssetResponse* response() const noexcept override;

private:
  CacheItem _cacheItem;
  CacheAssetResponse _response;
};

/**
 * @brief A decorator for an {@link IAssetAccessor} that caches requests and
 * responses in an {@link ICacheDatabase}.
 *
 * This can be used to improve asset loading performance by caching assets
 * across runs.
 */
class CachingAssetAccessor : public IAssetAccessor {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param pLogger The logger that receives messages about the status of this
   * instance.
   * @param pAssetAccessor The underlying {@link IAssetAccessor} used to
   * retrieve assets that are not in the cache.
   * @param pCacheDatabase The database in which to cache requests and
   * responses.
   * @param requestsPerCachePrune The number of requests to handle before each
   * {@link ICacheDatabase::prune} of old cached results from the database.
   */
  CachingAssetAccessor(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<ICacheDatabase>& pCacheDatabase,
      int32_t requestsPerCachePrune = 10000);

  virtual ~CachingAssetAccessor() noexcept override;

  /** @copydoc IAssetAccessor::get */
  virtual Future<std::shared_ptr<IAssetRequest>>
  get(const AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers) override;

  /**
   * @brief Same as above, but allows for disabling write-through into cache on
   * misses and revalidations. This may be useful if some derived client data
   * needs to be created from the response data before being cached. If
   * writeThrough is false writeBack(...) should be called later to explicitly
   * update the entry.
   *
   * @param asyncSystem The async system.
   * @param url The url to get from.
   * @param headers HTTP request headers.
   * @param writeThrough In the case of cache misses and revalidation, whether
   * to update the cache from the network response.
   *
   * @return Future<std::shared_ptr<IAssetRequest>>
   */
  Future<std::shared_ptr<IAssetRequest>>
  get(const AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers,
      bool writeThrough);

  /**
   * @brief Explicitly writes back an entry with client data derived from the
   * response into disk cache.
   *
   * @param asyncSystem The async system to use.
   * @param pCompletedRequest The completed network request.
   * @param cacheOriginalResponseData Whether to cache the original, response
   * data from the completed HTTP request.
   * @param clientData Arbitrary client data associated with this request.
   */
  Future<void> writeBack(
      const AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetRequest>& pCompletedRequest,
      bool cacheOriginalResponseData,
      std::vector<std::byte>&& clientData);

  virtual Future<std::shared_ptr<IAssetRequest>> request(
      const AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers,
      const gsl::span<const std::byte>& contentPayload) override;

  /** @copydoc IAssetAccessor::tick */
  virtual void tick() noexcept override;

private:
  int32_t _requestsPerCachePrune;
  std::atomic<int32_t> _requestSinceLastPrune;
  std::shared_ptr<spdlog::logger> _pLogger;
  std::shared_ptr<IAssetAccessor> _pAssetAccessor;
  std::shared_ptr<ICacheDatabase> _pCacheDatabase;
  ThreadPool _cacheThreadPool;
  CESIUM_TRACE_DECLARE_TRACK_SET(_pruneSlots, "Prune cache database");
};
} // namespace CesiumAsync
