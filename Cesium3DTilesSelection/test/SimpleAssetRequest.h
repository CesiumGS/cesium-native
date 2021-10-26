#pragma once

#include "SimpleAssetResponse.h"

#include <CesiumAsync/IAssetRequest.h>

#include <memory>

class SimpleAssetRequest : public CesiumAsync::IAssetRequest {
public:
  SimpleAssetRequest(
      const std::string& method,
      const std::string& url,
      const CesiumAsync::HttpHeaders& headers,
      std::unique_ptr<SimpleAssetResponse> pResponse)
      : requestMethod{method},
        requestUrl{url},
        requestHeaders{headers},
        pResponse{std::move(pResponse)} {}

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
