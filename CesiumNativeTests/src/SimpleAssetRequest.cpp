#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>

#include <memory>

namespace CesiumNativeTests {
std::unique_ptr<SimpleAssetRequest> createSuccessfulRequest(
    const std::string& method,
    const std::string& url,
    const CesiumAsync::HttpHeaders& headers,
    std::vector<std::byte>&& data) {
  return std::make_unique<SimpleAssetRequest>(
      method,
      url,
      headers,
      std::make_unique<SimpleAssetResponse>(
          uint16_t(200),
          method,
          headers,
          std::move(data)));
}

std::unique_ptr<SimpleAssetRequest> createFailedRequest(
    const std::string& method,
    const std::string& url,
    const CesiumAsync::HttpHeaders& headers) {
  return std::make_unique<SimpleAssetRequest>(
      method,
      url,
      headers,
      std::make_unique<SimpleAssetResponse>(
          uint16_t(404),
          method,
          headers,
          std::vector<std::byte>{}));
}
} // namespace CesiumNativeTests
