#pragma once

#include "SimpleAssetResponse.h"

#include <CesiumAsync/IAssetRequest.h>

#include <memory>

namespace Cesium3DTilesSelection {
class SimpleAssetRequest : public CesiumAsync::IAssetRequest {
public:
  SimpleAssetRequest(
      const std::string& method_,
      const std::string& url_,
      const CesiumAsync::HttpHeaders& headers_,
      std::unique_ptr<SimpleAssetResponse> pResponse_)
      : requestMethod{method_},
        requestUrl{url_},
        requestHeaders{headers_},
        pResponse{std::move(pResponse_)} {}

  virtual const std::string& method() const override {
    return this->requestMethod;
  }

  virtual const std::string& url() const override { return this->requestUrl; }

  virtual const CesiumAsync::HttpHeaders& headers() const override {
    return this->requestHeaders;
  }

  virtual const CesiumAsync::IAssetResponse* response() const override {
    return this->pResponse.get();
  }

  std::string requestMethod;
  std::string requestUrl;
  CesiumAsync::HttpHeaders requestHeaders;
  std::unique_ptr<SimpleAssetResponse> pResponse;
};
} // namespace Cesium3DTilesSelection
