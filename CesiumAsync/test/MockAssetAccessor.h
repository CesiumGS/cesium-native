#pragma once

#include "MockAssetRequest.h"
#include "MockAssetResponse.h"

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
      const std::string& url,
      const std::vector<THeader>& headers) override {
    auto it = responsesByUrl.find(url);
    if (it != responsesByUrl.end()) {
      std::unique_ptr<CesiumAsync::IAssetResponse> pResponse =
          std::make_unique<MockAssetResponse>(it->second);
      return asyncSystem
          .createResolvedFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
              std::make_shared<MockAssetRequest>(
                  "GET",
                  url,
                  CesiumAsync::HttpHeaders(headers.begin(), headers.end()),
                  std::move(pResponse)));
    } else {
      return asyncSystem.createResolvedFuture(
          std::shared_ptr<CesiumAsync::IAssetRequest>(testRequest));
    }
  }

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers,
      const std::span<const std::byte>& /* contentPayload */
      ) override {
    auto it = responsesByUrl.find(url);
    if (it != responsesByUrl.end()) {
      std::unique_ptr<CesiumAsync::IAssetResponse> pResponse =
          std::make_unique<MockAssetResponse>(it->second);
      return asyncSystem
          .createResolvedFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
              std::make_shared<MockAssetRequest>(
                  verb,
                  url,
                  CesiumAsync::HttpHeaders(headers.begin(), headers.end()),
                  std::move(pResponse)));
    } else {
      return asyncSystem.createResolvedFuture(
          std::shared_ptr<CesiumAsync::IAssetRequest>(testRequest));
    }
  }

  virtual void tick() noexcept override {}

  std::shared_ptr<CesiumAsync::IAssetRequest> testRequest;
  std::map<std::string, MockAssetResponse> responsesByUrl;
};
