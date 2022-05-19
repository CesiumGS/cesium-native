#pragma once

#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>

#include <cstddef>
#include <map>
#include <memory>

class SimpleAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
  SimpleAssetAccessor(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>&&
          mockCompletedRequests)
      : mockCompletedRequests{std::move(mockCompletedRequests)} {}

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const std::shared_ptr<CesiumAsync::AsyncSystem>& pAsyncSystem,
      const std::string& url,
      const std::vector<THeader>&) override {
    auto mockRequestIt = mockCompletedRequests.find(url);
    if (mockRequestIt != mockCompletedRequests.end()) {
      return pAsyncSystem->createResolvedFuture(
          std::shared_ptr<CesiumAsync::IAssetRequest>(mockRequestIt->second));
    }

    return pAsyncSystem->createResolvedFuture(
        std::shared_ptr<CesiumAsync::IAssetRequest>(nullptr));
  }

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  request(
      const std::shared_ptr<CesiumAsync::AsyncSystem>& pAsyncSystem,
      const std::string& /* verb */,
      const std::string& url,
      const std::vector<THeader>& headers,
      const gsl::span<const std::byte>&) override {
    return this->get(asyncSystem, url, headers);
  }

  virtual void tick() noexcept override {}

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
};
