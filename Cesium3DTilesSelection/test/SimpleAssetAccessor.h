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
      const std::vector<THeader>&) override {
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
    return this->get(asyncSystem, url, headers);
  }

  virtual void tick() noexcept override {}

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
};
} // namespace Cesium3DTilesSelection
