#pragma once

#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>

#include <memory>
#include <string>

class MockAssetRequest : public CesiumAsync::IAssetRequest {
public:
  MockAssetRequest(
      const std::string& method,
      const std::string& url,
      const CesiumAsync::HttpHeaders& headers,
      std::unique_ptr<CesiumAsync::IAssetResponse> response)
      : _method{method},
        _url{url},
        _headers{headers},
        _pResponse{std::move(response)} {}

  virtual const std::string& method() const override { return this->_method; }

  virtual const std::string& url() const override { return this->_url; }

  virtual const CesiumAsync::HttpHeaders& headers() const override {
    return this->_headers;
  }

  virtual const CesiumAsync::IAssetResponse* response() const override {
    return this->_pResponse.get();
  }

private:
  std::string _method;
  std::string _url;
  CesiumAsync::HttpHeaders _headers;
  std::unique_ptr<CesiumAsync::IAssetResponse> _pResponse;
};
