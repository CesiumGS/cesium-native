#include "CesiumAsync/GunzipAssetAccessor.h"

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumUtility/Gunzip.h"

namespace CesiumAsync {
class GunzippedAssetResponse : public IAssetResponse {
public:
  GunzippedAssetResponse(const IAssetResponse* pOther) noexcept
      : _pAssetResponse{pOther} {
    _dataValid = CesiumUtility::gunzip(
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

  virtual gsl::span<const std::byte> data() const noexcept override {
    return _dataValid ? gsl::span(_gunzippedData.data(), _gunzippedData.size())
                      : _pAssetResponse->data();
  }

private:
  const IAssetResponse* _pAssetResponse;
  std::vector<std::byte> _gunzippedData;
  bool _dataValid;
};

class GunzippedAssetRequest : public IAssetRequest {
public:
  GunzippedAssetRequest(std::shared_ptr<IAssetRequest>& pOther)
      : _pAssetRequest(pOther), _AssetResponse(pOther->response()){};
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
    return &this->_AssetResponse;
  }

private:
  std::shared_ptr<IAssetRequest> _pAssetRequest;
  GunzippedAssetResponse _AssetResponse;
};

GunzipAssetAccessor::GunzipAssetAccessor(
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor)
    : _pAssetAccessor(pAssetAccessor) {}

GunzipAssetAccessor::~GunzipAssetAccessor() noexcept {}

Future<std::shared_ptr<IAssetRequest>> GunzipAssetAccessor::get(
    const AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  return this->_pAssetAccessor->get(asyncSystem, url, headers)
      .thenImmediately([](std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
        const IAssetResponse* pResponse = pCompletedRequest->response();
        if (!pResponse) {
          return std::move(pCompletedRequest);
        }
        if (CesiumUtility::isGzip(pResponse->data())) {
          pCompletedRequest = std::make_shared<GunzippedAssetRequest>(
              GunzippedAssetRequest(pCompletedRequest));
        }
        return std::move(pCompletedRequest);
      });
}

Future<std::shared_ptr<IAssetRequest>> GunzipAssetAccessor::request(
    const AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const gsl::span<const std::byte>& contentPayload) {
  return this->_pAssetAccessor
      ->request(asyncSystem, verb, url, headers, contentPayload);
}

void GunzipAssetAccessor::tick() noexcept { _pAssetAccessor->tick(); }
} // namespace CesiumAsync
