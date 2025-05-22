#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumUtility/Uri.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <memory>

std::string randomStringOfLen(size_t len);
std::string generateAuthToken(bool isAccessToken);

class MockITwinAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
  MockITwinAssetAccessor(bool isAccessToken)
      : authToken(generateAuthToken(isAccessToken)),
        refreshToken(randomStringOfLen(42)) {}

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>&) override;

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& /* verb */,
      const std::string& url,
      const std::vector<THeader>& headers,
      const std::span<const std::byte>&) override;

  virtual void tick() noexcept override {}

  std::string authToken;
  std::optional<std::string> refreshToken;

private:
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  handleAuthServer(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const CesiumUtility::Uri& url,
      const std::vector<THeader>& headers,
      const std::vector<std::byte>& body);

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  handleApiServer(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const CesiumUtility::Uri& url,
      const std::vector<THeader>& headers,
      const std::vector<std::byte>& body);
};
