#include "InternalTimegm.h"
#include "ResponseCacheControl.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/CacheItem.h>
#include <CesiumAsync/CachingAssetAccessor.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/ICacheDatabase.h>
#include <CesiumUtility/Tracing.h>

#include <spdlog/logger.h>

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace CesiumAsync {
class CacheAssetResponse : public IAssetResponse {
public:
  CacheAssetResponse(CacheResponse&& cacheResponse) noexcept
      : _cacheResponse(std::move(cacheResponse)) {}

  virtual uint16_t statusCode() const noexcept override {
    return this->_cacheResponse.statusCode;
  }

  virtual std::string contentType() const override {
    auto it = this->_cacheResponse.headers.find("Content-Type");
    if (it == this->_cacheResponse.headers.end()) {
      return std::string();
    }
    return it->second;
  }

  virtual const HttpHeaders& headers() const noexcept override {
    return this->_cacheResponse.headers;
  }

  virtual std::span<const std::byte> data() const noexcept override {
    return std::span<const std::byte>(
        this->_cacheResponse.data.data(),
        this->_cacheResponse.data.size());
  }

private:
  CacheResponse _cacheResponse;
};

class CacheAssetRequest : public IAssetRequest {
public:
  CacheAssetRequest(
      std::string&& url,
      HttpHeaders&& headers,
      CacheItem&& cacheItem)
      : _method(std::move(cacheItem.cacheRequest.method)),
        _url(std::move(url)),
        _headers(std::move(headers)),
        _response(std::move(cacheItem.cacheResponse)) {}

  virtual const std::string& method() const noexcept override {
    return this->_method;
  }

  virtual const std::string& url() const noexcept override {
    return this->_url;
  }

  virtual const HttpHeaders& headers() const noexcept override {
    return this->_headers;
  }

  virtual const IAssetResponse* response() const noexcept override {
    return &this->_response;
  }

private:
  std::string _method;
  std::string _url;
  HttpHeaders _headers;
  CacheAssetResponse _response;
};

namespace {

std::time_t convertHttpDateToTime(const std::string& httpDate);

bool shouldRevalidateCache(const CacheItem& cacheItem);

bool isCacheStale(const CacheItem& cacheItem) noexcept;

bool shouldCacheRequest(
    const IAssetRequest& request,
    const std::optional<ResponseCacheControl>& cacheControl);

std::string calculateCacheKey(const IAssetRequest& request);

std::time_t calculateExpiryTime(
    const IAssetRequest& request,
    const std::optional<ResponseCacheControl>& cacheControl);

std::shared_ptr<IAssetRequest> updateCacheItem(
    std::string&& url,
    std::vector<IAssetAccessor::THeader>&& headers,
    CacheItem&& cacheItem,
    const IAssetRequest& request);

} // namespace

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

CachingAssetAccessor::~CachingAssetAccessor() noexcept = default;

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
           url = url,
           headers = headers,
           threadPool]() mutable -> Future<std::shared_ptr<IAssetRequest>> {
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
              HttpHeaders::const_iterator etagHeader =
                  responseHeaders.find("Etag");
              if (etagHeader != responseHeaders.end()) {
                newHeaders.emplace_back("If-None-Match", etagHeader->second);
              } else {
                HttpHeaders::const_iterator lastModifiedHeader =
                    responseHeaders.find("Last-Modified");
                if (lastModifiedHeader != responseHeaders.end())
                  newHeaders.emplace_back(
                      "If-Modified-Since",
                      lastModifiedHeader->second);
              }

              return pAssetAccessor->get(asyncSystem, url, newHeaders)
                  .thenInThreadPool(
                      threadPool,
                      [cacheItem = std::move(cacheItem),
                       pCacheDatabase,
                       pLogger,
                       url = std::move(url),
                       headers =
                           std::move(headers)](std::shared_ptr<IAssetRequest>&&
                                                   pCompletedRequest) mutable {
                        if (!pCompletedRequest) {
                          return std::move(pCompletedRequest);
                        }

                        std::shared_ptr<IAssetRequest> pRequestToStore;
                        if (pCompletedRequest->response()->statusCode() ==
                            304) { // status Not-Modified
                          pRequestToStore = updateCacheItem(
                              std::move(url),
                              std::move(headers),
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
                std::make_shared<CacheAssetRequest>(
                    std::move(url),
                    HttpHeaders(
                        std::make_move_iterator(headers.begin()),
                        std::make_move_iterator(headers.end())),
                    std::move(cacheItem));
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
    const std::span<const std::byte>& contentPayload) {
  return this->_pAssetAccessor
      ->request(asyncSystem, verb, url, headers, contentPayload);
}

void CachingAssetAccessor::tick() noexcept { _pAssetAccessor->tick(); }

namespace {

bool shouldRevalidateCache(const CacheItem& cacheItem) {
  std::optional<ResponseCacheControl> cacheControl =
      ResponseCacheControl::parseFromResponseHeaders(
          cacheItem.cacheResponse.headers);
  if (cacheControl && cacheControl->noCache())
    return true;

  // Always revalidate if cache is stale. We always assume online scenarios.
  // A must-revalidate directive doesn't change this logic.
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

  const HttpHeaders& headers = pResponse->headers();
  HttpHeaders::const_iterator expiresHeader = headers.find("Expires");
  bool expiresExists = expiresHeader != headers.end();

  //
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cache-Control
  //
  // The no-store response directive indicates that any caches of any kind
  // (private or shared) should not store this response.
  if (cacheControl && cacheControl->noStore())
    return false;

  //
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Expires
  //
  // If there is a Cache-Control header with the max-age or s-maxage directive
  // in the response, the Expires header is ignored
  bool preferCacheControl = false;

  // If Cache-Control expiration is in the future, definitely cache. But we
  // might be able to cache even if it's not.
  if (cacheControl) {
    if (cacheControl->maxAgeExists()) {
      preferCacheControl = true;
      if (cacheControl->maxAgeValue() > 0)
        return true;
    } else if (cacheControl->sharedMaxAgeExists()) {
      preferCacheControl = true;
      if (cacheControl->sharedMaxAgeValue() > 0)
        return true;
    }
  }

  // If Expires is in the future, definitely cache. But we might be able to
  // cache even if it's not.
  if (!preferCacheControl && expiresExists) {
    bool alreadyExpired = std::difftime(
                              convertHttpDateToTime(expiresHeader->second),
                              std::time(nullptr)) <= 0.0;
    if (!alreadyExpired)
      return true;
  }

  // If we have a way to revalidate, we can store
  bool hasEtag = headers.find("ETag") != headers.end();
  bool hasLastModifiedEtag = headers.find("Last-Modified") != headers.end();
  if (hasEtag || hasLastModifiedEtag)
    return true;

  // Else don't store it
  return false;
}

std::string calculateCacheKey(const IAssetRequest& request) {
  // TODO: more complete cache key
  return request.url();
}

std::time_t calculateExpiryTime(
    const IAssetRequest& request,
    const std::optional<ResponseCacheControl>& cacheControl) {

  //
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Expires
  //
  // If there is a Cache-Control header with the max-age or s-maxage directive
  // in the response, the Expires header is ignored
  bool preferCacheControl =
      cacheControl &&
      (cacheControl->maxAgeExists() || cacheControl->sharedMaxAgeExists());

  if (preferCacheControl) {
    int maxAgeValue =
        cacheControl->maxAgeExists() ? cacheControl->maxAgeValue() : 0;
    return std::time(nullptr) + maxAgeValue;
  } else {
    const IAssetResponse* pResponse = request.response();
    const HttpHeaders& responseHeaders = pResponse->headers();
    HttpHeaders::const_iterator expiresHeader = responseHeaders.find("Expires");
    if (expiresHeader != responseHeaders.end())
      return convertHttpDateToTime(expiresHeader->second);

    return std::time(nullptr);
  }
}

std::shared_ptr<IAssetRequest> updateCacheItem(
    std::string&& url,
    std::vector<IAssetAccessor::THeader>&& headers,
    CacheItem&& cacheItem,
    const IAssetRequest& request) {
  const IAssetResponse* pResponse = request.response();
  if (pResponse) {
    // Copy the response headers from the new request into the cacheItem so that
    // they're included in the new response. This is particularly important for
    // Expires headers and the like.
    for (const auto& pair : pResponse->headers()) {
      cacheItem.cacheResponse.headers[pair.first] = pair.second;
    }
  }

  return std::make_shared<CacheAssetRequest>(
      std::move(url),
      HttpHeaders(
          std::make_move_iterator(headers.begin()),
          std::make_move_iterator(headers.end())),
      std::move(cacheItem));
}

std::time_t convertHttpDateToTime(const std::string& httpDate) {
  std::tm tm = {};
  std::stringstream ss(httpDate);
  ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S");
  return internalTimegm(&tm);
}

} // namespace

} // namespace CesiumAsync
