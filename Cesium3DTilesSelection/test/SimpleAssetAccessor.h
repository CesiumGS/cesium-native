#pragma once

#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>

#include <catch2/catch.hpp>

#include <cstddef>
#include <map>
#include <memory>

namespace Cesium3DTilesSelection {
class SimpleAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
  SimpleAssetAccessor(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>&&
          mockCompletedRequests)
      : mockCompletedRequests{std::move(mockCompletedRequests)} {}

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>&,
      bool /*writeThrough*/) override {
    auto mockRequestIt = mockCompletedRequests.find(url);
    if (mockRequestIt != mockCompletedRequests.end()) {
      return asyncSystem.createResolvedFuture(
          std::shared_ptr<CesiumAsync::IAssetRequest>(mockRequestIt->second));
    }

    FAIL("Cannot find request for url " << url);

    return asyncSystem.createResolvedFuture(
        std::shared_ptr<CesiumAsync::IAssetRequest>(nullptr));
  }

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& /* verb */,
      const std::string& url,
      const std::vector<THeader>& headers,
      const gsl::span<const std::byte>&) override {
    return this->get(asyncSystem, url, headers, true);
  }

  virtual void tick() noexcept override {}

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
};

class SimpleCachedAssetAccessor : public CesiumAsync::IAssetAccessor {
private:

public:
  std::map<std::string, std::shared_ptr<CesiumAsync::IAssetRequest>> mockCache;
  std::shared_ptr<SimpleAssetAccessor> underlyingAssetAccessor;

  SimpleCachedAssetAccessor(
      const std::shared_ptr<SimpleAssetAccessor>& underlyingAssetAccessor_,
      std::map<std::string, std::shared_ptr<CesiumAsync::IAssetRequest>>&& mockCache_)
    : underlyingAssetAccessor(underlyingAssetAccessor_),
      mockCache(std::move(mockCache_)) {}

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>&,
      bool writeThrough) override {
    auto mockCacheHit = this->mockCache.find(url);
    if (mockCacheHit != this->mockCache.end()) {
      return asyncSystem.createResolvedFuture(
          std::shared_ptr<CesiumAsync::IAssetRequest>(mockCacheHit->second));
    }

    std::shared_ptr<CesiumAsync::IAssetRequest> pNewRequest = 
        this->underlyingAssetAccessor->get(asyncSystem, url, {}, true).wait();
    const CesiumAsync::IAssetResponse* pNewResponse = pNewRequest->response();
    gsl::span<const std::byte> responseData = pNewResponse->data();

    if (writeThrough) {
      // Create a deep copy of the request to store in the cache, to mock 
      // future cache responses.
      auto pCacheResponse = std::make_unique<SimpleAssetResponse>(
            pNewResponse->statusCode(),
            pNewResponse->contentType(), 
            pNewResponse->headers(), 
            std::vector<std::byte>(responseData.begin(), responseData.end()),
            std::vector<std::byte>());
      auto pCacheEntry = std::make_shared<SimpleAssetRequest>(
              pNewRequest->method(), 
              pNewRequest->url(), 
              pNewRequest->headers(), 
              std::move(pCacheResponse));
      this->mockCache.emplace(url, std::move(pCacheEntry));
    }

    return asyncSystem.createResolvedFuture(std::move(pNewRequest));
  }

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& /* verb */,
      const std::string& url,
      const std::vector<THeader>& headers,
      const gsl::span<const std::byte>&) override {
    return this->get(asyncSystem, url, headers, true);
  }

  virtual CesiumAsync::Future<void> writeBack(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetRequest>& pCompletedRequest,
      bool cacheOriginalResponseData,
      std::vector<std::byte>&& clientData) {

    const CesiumAsync::IAssetResponse* pCompletedResponse = 
        pCompletedRequest->response();
    gsl::span<const std::byte> responseData = pCompletedResponse->data();
    
    auto pCacheResponse = std::make_unique<SimpleAssetResponse>(
          pCompletedResponse->statusCode(),
          pCompletedResponse->contentType(), 
          pCompletedResponse->headers(), 
          cacheOriginalResponseData ? 
              std::vector<std::byte>(responseData.begin(), responseData.end()) :
              std::vector<std::byte>(),
          std::move(clientData));
    auto pCacheEntry = std::make_shared<SimpleAssetRequest>(
            pCompletedRequest->method(), 
            pCompletedRequest->url(), 
            pCompletedRequest->headers(), 
            std::move(pCacheResponse));
    this->mockCache.emplace(
        pCompletedRequest->url(), 
        std::move(pCacheEntry));


    return asyncSystem.createResolvedFuture();
  }
  virtual void tick() noexcept override {}
};
} // namespace Cesium3DTilesSelection
