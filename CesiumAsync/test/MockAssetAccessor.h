#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <memory>
#include <string>
#include <vector>

class MockAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
  MockAssetAccessor(const std::shared_ptr<CesiumAsync::IAssetRequest>& request)
      : testRequest{request} {}

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& /* url */,
      const std::vector<THeader>& /* headers */
      ) override {
    return asyncSystem.createResolvedFuture(
        std::shared_ptr<CesiumAsync::IAssetRequest>(testRequest));
  }

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& /* verb */,
      const std::string& /* url */,
      const std::vector<THeader>& /* headers */,
      const std::span<const std::byte>& /* contentPayload */
      ) override {
    return asyncSystem.createResolvedFuture(
        std::shared_ptr<CesiumAsync::IAssetRequest>(testRequest));
  }

  virtual void tick() noexcept override {}

  std::shared_ptr<CesiumAsync::IAssetRequest> testRequest;
};
