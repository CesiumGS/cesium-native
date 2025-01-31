#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/GunzipAssetAccessor.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Gzip.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace CesiumAsync {

namespace {

class GunzippedAssetResponse : public IAssetResponse {
public:
  GunzippedAssetResponse(const IAssetResponse* pOther) noexcept
      : _pAssetResponse{pOther} {
    this->_dataValid = CesiumUtility::gunzip(
        this->_pAssetResponse->data(),
        this->_gunzippedData);
  }

  virtual uint16_t statusCode() const noexcept override {
    return this->_pAssetResponse->statusCode();
  }

  virtual std::string contentType() const override {
    return this->_pAssetResponse->contentType();
  }

  virtual const HttpHeaders& headers() const noexcept override {
    return this->_pAssetResponse->headers();
  }

  virtual std::span<const std::byte> data() const noexcept override {
    return this->_dataValid ? this->_gunzippedData
                            : this->_pAssetResponse->data();
  }

private:
  const IAssetResponse* _pAssetResponse;
  std::vector<std::byte> _gunzippedData;
  bool _dataValid;
};

class GunzippedAssetRequest : public IAssetRequest {
public:
  GunzippedAssetRequest(std::shared_ptr<IAssetRequest>&& pOther)
      : _pAssetRequest(std::move(pOther)),
        _assetResponse(_pAssetRequest->response()){};
  virtual const std::string& method() const noexcept override {
    return this->_pAssetRequest->method();
  }

  virtual const std::string& url() const noexcept override {
    return this->_pAssetRequest->url();
  }

  virtual const HttpHeaders& headers() const noexcept override {
    return this->_pAssetRequest->headers();
  }

  virtual const IAssetResponse* response() const noexcept override {
    return &this->_assetResponse;
  }

private:
  std::shared_ptr<IAssetRequest> _pAssetRequest;
  GunzippedAssetResponse _assetResponse;
};

Future<std::shared_ptr<IAssetRequest>> gunzipIfNeeded(
    const AsyncSystem& asyncSystem,
    std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
  const IAssetResponse* pResponse = pCompletedRequest->response();
  if (pResponse && CesiumUtility::isGzip(pResponse->data())) {
    return asyncSystem.runInWorkerThread(
        [pCompletedRequest = std::move(
             pCompletedRequest)]() mutable -> std::shared_ptr<IAssetRequest> {
          return std::make_shared<GunzippedAssetRequest>(
              std::move(pCompletedRequest));
        });
  }
  return asyncSystem.createResolvedFuture(std::move(pCompletedRequest));
}

} // namespace

GunzipAssetAccessor::GunzipAssetAccessor(
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor)
    : _pAssetAccessor(pAssetAccessor) {}

GunzipAssetAccessor::~GunzipAssetAccessor() noexcept = default;

Future<std::shared_ptr<IAssetRequest>> GunzipAssetAccessor::get(
    const AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  return this->_pAssetAccessor->get(asyncSystem, url, headers)
      .thenImmediately(
          [asyncSystem](std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
            return gunzipIfNeeded(asyncSystem, std::move(pCompletedRequest));
          });
}

Future<std::shared_ptr<IAssetRequest>> GunzipAssetAccessor::request(
    const AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const std::span<const std::byte>& contentPayload) {
  return this->_pAssetAccessor
      ->request(asyncSystem, verb, url, headers, contentPayload)
      .thenImmediately(
          [asyncSystem](std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
            return gunzipIfNeeded(asyncSystem, std::move(pCompletedRequest));
          });
}

void GunzipAssetAccessor::tick() noexcept { _pAssetAccessor->tick(); }

} // namespace CesiumAsync
