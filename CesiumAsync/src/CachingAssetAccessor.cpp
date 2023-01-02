#include "CesiumAsync/CachingAssetAccessor.h"

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/CacheItem.h"
#include "CesiumAsync/IAssetResponse.h"
#include "InternalTimegm.h"
#include "ResponseCacheControl.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>

namespace CesiumAsync {
class CacheAssetResponse : public IAssetResponse {
public:
  CacheAssetResponse(const CacheItem* pCacheItem) noexcept
      : _pCacheItem{pCacheItem} {}

  virtual uint16_t statusCode() const noexcept override {
    return this->_pCacheItem->cacheResponse.statusCode;
  }

  virtual std::string contentType() const override {
    auto it = this->_pCacheItem->cacheResponse.headers.find("Content-Type");
    if (it == this->_pCacheItem->cacheResponse.headers.end()) {
      return std::string();
    }
    return it->second;
  }

  virtual const HttpHeaders& headers() const noexcept override {
    return this->_pCacheItem->cacheResponse.headers;
  }

  virtual gsl::span<const std::byte> data() const noexcept override {
    return gsl::span<const std::byte>(
        this->_pCacheItem->cacheResponse.data.data(),
        this->_pCacheItem->cacheResponse.data.size());
  }

private:
  const CacheItem* _pCacheItem;
};

class CacheAssetRequest : public IAssetRequest {
public:
  CacheAssetRequest(CacheItem&& cacheItem)
      : _cacheItem(std::move(cacheItem)), _response(&this->_cacheItem) {}

  virtual const std::string& method() const noexcept override {
    return this->_cacheItem.cacheRequest.method;
  }

  virtual const std::string& url() const noexcept override {
    return this->_cacheItem.cacheRequest.url;
  }

  virtual const HttpHeaders& headers() const noexcept override {
    return this->_cacheItem.cacheRequest.headers;
  }

  virtual const IAssetResponse* response() const noexcept override {
    return &this->_response;
  }

private:
  CacheItem _cacheItem;
  CacheAssetResponse _response;
};

static std::time_t convertHttpDateToTime(const std::string& httpDate);

static bool shouldRevalidateCache(const CacheItem& cacheItem);

static bool isCacheStale(const CacheItem& cacheItem) noexcept;

static bool shouldCacheRequest(
    const IAssetRequest& request,
    const std::optional<ResponseCacheControl>& cacheControl);

static std::string calculateCacheKey(const IAssetRequest& request);

static std::time_t calculateExpiryTime(
    const IAssetRequest& request,
    const std::optional<ResponseCacheControl>& cacheControl);

static std::unique_ptr<IAssetRequest>
updateCacheItem(CacheItem&& cacheItem, const IAssetRequest& request);

CachingAssetAccessor::CachingAssetAccessor(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<ICacheDatabase>& pCacheDatabase,
    int32_t requestsPerCachePrune)
    : _requestsPerCachePrune(requestsPerCachePrune),
      _requestSinceLastPrune(0),
      _pLogger(pLogger),
      _pAssetAccessor(pAssetAccessor),
      _pCacheDatabase(pCacheDatabase),
      _cacheThreadPool(1) {}

CachingAssetAccessor::~CachingAssetAccessor() noexcept {}

Future<std::shared_ptr<IAssetRequest>> CachingAssetAccessor::get(
    const AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  const int32_t requestSinceLastPrune = ++this->_requestSinceLastPrune;
  if (requestSinceLastPrune == this->_requestsPerCachePrune) {
    // More requests may have started and incremented _requestSinceLastPrune
    // beyond _requestsPerCachePrune before this next line. That's ok.
    this->_requestSinceLastPrune = 0;

    CESIUM_TRACE_USE_TRACK_SET(this->_pruneSlots);
    asyncSystem.runInThreadPool(this->_cacheThreadPool, [this]() {
      this->_pCacheDatabase->prune();
    });
  }

  CESIUM_TRACE_BEGIN_IN_TRACK("IAssetAccessor::get (cached)");

  const ThreadPool& threadPool = this->_cacheThreadPool;

  return asyncSystem
      .runInThreadPool(
          this->_cacheThreadPool,
          [asyncSystem,
           pAssetAccessor = this->_pAssetAccessor,
           pCacheDatabase = this->_pCacheDatabase,
           pLogger = this->_pLogger,
           url,
           headers,
           threadPool]() -> Future<std::shared_ptr<IAssetRequest>> {
            std::optional<CacheItem> cacheLookup =
                pCacheDatabase->getEntry(url);
            if (!cacheLookup) {
              // No cache item found, request directly from the server
              return pAssetAccessor->get(asyncSystem, url, headers)
                  .thenInThreadPool(
                      threadPool,
                      [pCacheDatabase, pLogger](
                          std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
                        const IAssetResponse* pResponse =
                            pCompletedRequest->response();
                        if (!pResponse) {
                          return std::move(pCompletedRequest);
                        }

                        const std::optional<ResponseCacheControl> cacheControl =
                            ResponseCacheControl::parseFromResponseHeaders(
                                pResponse->headers());

                        if (pResponse && shouldCacheRequest(
                                             *pCompletedRequest,
                                             cacheControl)) {
                          pCacheDatabase->storeEntry(
                              calculateCacheKey(*pCompletedRequest),
                              calculateExpiryTime(
                                  *pCompletedRequest,
                                  cacheControl),
                              pCompletedRequest->url(),
                              pCompletedRequest->method(),
                              pCompletedRequest->headers(),
                              pResponse->statusCode(),
                              pResponse->headers(),
                              pResponse->data());
                        }

                        return std::move(pCompletedRequest);
                      });
            }

            CacheItem& cacheItem = cacheLookup.value();

            if (shouldRevalidateCache(cacheItem)) {
              // Cache is stale and needs revalidation
              std::vector<THeader> newHeaders = headers;
              const CacheResponse& cacheResponse = cacheItem.cacheResponse;
              const HttpHeaders& responseHeaders = cacheResponse.headers;
              HttpHeaders::const_iterator lastModifiedHeader =
                  responseHeaders.find("Last-Modified");
              HttpHeaders::const_iterator etagHeader =
                  responseHeaders.find("Etag");
              if (etagHeader != responseHeaders.end()) {
                newHeaders.emplace_back("If-None-Match", etagHeader->second);
              } else if (lastModifiedHeader != responseHeaders.end()) {
                newHeaders.emplace_back(
                    "If-Modified-Since",
                    lastModifiedHeader->second);
              }

              return pAssetAccessor->get(asyncSystem, url, newHeaders)
                  .thenInThreadPool(
                      threadPool,
                      [cacheItem = std::move(cacheItem),
                       pCacheDatabase,
                       pLogger](std::shared_ptr<IAssetRequest>&&
                                    pCompletedRequest) mutable {
                        if (!pCompletedRequest) {
                          return std::move(pCompletedRequest);
                        }

                        std::shared_ptr<IAssetRequest> pRequestToStore;
                        if (pCompletedRequest->response()->statusCode() ==
                            304) { // status Not-Modified
                          pRequestToStore = updateCacheItem(
                              std::move(cacheItem),
                              *pCompletedRequest);
                        } else {
                          pRequestToStore = pCompletedRequest;
                        }

                        const IAssetResponse* pResponseToStore =
                            pRequestToStore->response();
                        const std::optional<ResponseCacheControl> cacheControl =
                            ResponseCacheControl::parseFromResponseHeaders(
                                pResponseToStore->headers());

                        if (shouldCacheRequest(
                                *pRequestToStore,
                                cacheControl)) {

                          pCacheDatabase->storeEntry(
                              calculateCacheKey(*pRequestToStore),
                              calculateExpiryTime(
                                  *pRequestToStore,
                                  cacheControl),
                              pRequestToStore->url(),
                              pRequestToStore->method(),
                              pRequestToStore->headers(),
                              pResponseToStore->statusCode(),
                              pResponseToStore->headers(),
                              pResponseToStore->data());
                        }

                        return pRequestToStore;
                      });
            }

            // Good cache item that doesn't need to be revalidated, just return
            // it.
            std::shared_ptr<IAssetRequest> pRequest =
                std::make_shared<CacheAssetRequest>(std::move(cacheItem));
            return asyncSystem.createResolvedFuture(std::move(pRequest));
          })
      .thenImmediately([](std::shared_ptr<IAssetRequest>&& pRequest) noexcept {
        CESIUM_TRACE_END_IN_TRACK("IAssetAccessor::get (cached)");
        return std::move(pRequest);
      });
}

Future<std::shared_ptr<IAssetRequest>> CachingAssetAccessor::request(
    const AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const gsl::span<const std::byte>& contentPayload) {
  return this->_pAssetAccessor
      ->request(asyncSystem, verb, url, headers, contentPayload);
}

void CachingAssetAccessor::tick() noexcept { _pAssetAccessor->tick(); }

bool shouldRevalidateCache(const CacheItem& cacheItem) {
  std::optional<ResponseCacheControl> cacheControl =
      ResponseCacheControl::parseFromResponseHeaders(
          cacheItem.cacheResponse.headers);
  if (cacheControl) {
    if (isCacheStale(cacheItem) && cacheControl->mustRevalidate()) {
      return true;
    }
  }

  return isCacheStale(cacheItem);
}

bool isCacheStale(const CacheItem& cacheItem) noexcept {
  const std::time_t currentTime = std::time(nullptr);
  return std::difftime(cacheItem.expiryTime, currentTime) < 0.0;
}

bool shouldCacheRequest(
    const IAssetRequest& request,
    const std::optional<ResponseCacheControl>& cacheControl) {
  // no response then no cache
  const IAssetResponse* pResponse = request.response();
  if (!pResponse) {
    return false;
  }

  // only cache GET method
  if (request.method() != "GET") {
    return false;
  }

  // check if response status code is cacheable
  const uint16_t statusCode = pResponse->statusCode();
  if (statusCode != 200 && // status OK
      statusCode != 201 && // status Created
      statusCode != 202 && // status Accepted
      statusCode != 203 && // status Non-Authoritive Information
      statusCode != 204 && // status No-Content
      statusCode != 205 && // status Reset-Content
      statusCode != 304)   // status Not-Modifed
  {
    return false;
  }

  // check if cache control contains no-store or no-cache directives
  int maxAge = 0;
  if (cacheControl) {
    if (cacheControl->noStore() || cacheControl->noCache()) {
      return false;
    }

    maxAge = cacheControl->maxAge();
  }

  // check response header contains expires if maxAge is not specified
  const HttpHeaders& responseHeaders = pResponse->headers();
  if (maxAge == 0) {
    HttpHeaders::const_iterator expiresHeader = responseHeaders.find("Expires");
    if (expiresHeader == responseHeaders.end()) {
      return false;
    }

    return std::difftime(
               convertHttpDateToTime(expiresHeader->second),
               std::time(nullptr)) > 0.0;
  }

  return true;
}

std::string calculateCacheKey(const IAssetRequest& request) {
  // TODO: more complete cache key
  return request.url();
}

std::time_t calculateExpiryTime(
    const IAssetRequest& request,
    const std::optional<ResponseCacheControl>& cacheControl) {
  if (cacheControl) {
    if (cacheControl->maxAge() != 0) {
      return std::time(nullptr) + cacheControl->maxAge();
    }
  }

  const IAssetResponse* pResponse = request.response();
  const HttpHeaders& responseHeaders = pResponse->headers();
  HttpHeaders::const_iterator expiresHeader = responseHeaders.find("Expires");
  if (expiresHeader != responseHeaders.end()) {
    return convertHttpDateToTime(expiresHeader->second);
  }

  return std::time(nullptr);
}

std::unique_ptr<IAssetRequest>
updateCacheItem(CacheItem&& cacheItem, const IAssetRequest& request) {
  for (const std::pair<const std::string, std::string>& header :
       request.headers()) {
    cacheItem.cacheRequest.headers[header.first] = header.second;
  }

  const IAssetResponse* pResponse = request.response();
  if (pResponse) {
    for (const std::pair<const std::string, std::string>& header :
         pResponse->headers()) {
      cacheItem.cacheResponse.headers[header.first] = header.second;
    }
  }

  return std::make_unique<CacheAssetRequest>(std::move(cacheItem));
}

std::time_t convertHttpDateToTime(const std::string& httpDate) {
  std::tm tm = {};
  std::stringstream ss(httpDate);
  ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S");
  return internalTimegm(&tm);
}
} // namespace CesiumAsync
