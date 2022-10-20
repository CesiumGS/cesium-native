#pragma once

#include <CesiumAsync/IAssetResponse.h>

#include <cstddef>
#include <vector>

namespace Cesium3DTilesSelection {
class SimpleAssetResponse : public CesiumAsync::IAssetResponse {
public:
  SimpleAssetResponse(
      uint16_t statusCode,
      const std::string& contentType,
      const CesiumAsync::HttpHeaders& headers,
      const std::vector<std::byte>& data,
      const std::vector<std::byte>& clientData = {})
      : mockStatusCode{statusCode},
        mockContentType{contentType},
        mockHeaders{headers},
        mockData{data},
        mockClientData{clientData} {}

  virtual uint16_t statusCode() const override { return this->mockStatusCode; }

  virtual std::string contentType() const override {
    return this->mockContentType;
  }

  virtual const CesiumAsync::HttpHeaders& headers() const override {
    return this->mockHeaders;
  }

  virtual gsl::span<const std::byte> data() const override {
    return gsl::span<const std::byte>(mockData.data(), mockData.size());
  }

  virtual gsl::span<const std::byte> clientData() const override {
    return gsl::span<const std::byte>(mockClientData.data(), mockClientData.size());;
  }

  uint16_t mockStatusCode;
  std::string mockContentType;
  CesiumAsync::HttpHeaders mockHeaders;
  std::vector<std::byte> mockData;
  std::vector<std::byte> mockClientData;
};
} // namespace Cesium3DTilesSelection
