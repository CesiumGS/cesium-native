#pragma once

#include <CesiumAsync/IAssetResponse.h>

#include <cstddef>
#include <vector>

namespace CesiumNativeTests {
class SimpleAssetResponse : public CesiumAsync::IAssetResponse {
public:
  SimpleAssetResponse(
      uint16_t statusCode,
      const std::string& contentType,
      const CesiumAsync::HttpHeaders& headers,
      const std::vector<std::byte>& data)
      : mockStatusCode{statusCode},
        mockContentType{contentType},
        mockHeaders{headers},
        mockData{data} {}

  virtual uint16_t statusCode() const override { return this->mockStatusCode; }

  virtual std::string contentType() const override {
    return this->mockContentType;
  }

  virtual const CesiumAsync::HttpHeaders& headers() const override {
    return this->mockHeaders;
  }

  virtual std::span<const std::byte> data() const override {
    return std::span<const std::byte>(mockData.data(), mockData.size());
  }

  uint16_t mockStatusCode;
  std::string mockContentType;
  CesiumAsync::HttpHeaders mockHeaders;
  std::vector<std::byte> mockData;
};
} // namespace CesiumNativeTests
