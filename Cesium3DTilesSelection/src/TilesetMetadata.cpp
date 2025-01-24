#include <Cesium3DTilesReader/SchemaReader.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/Promise.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumUtility/joinToString.h>

#include <fmt/format.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

using namespace CesiumAsync;
using namespace Cesium3DTilesReader;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

TilesetMetadata::~TilesetMetadata() noexcept {
  if (this->_pLoadingCanceled) {
    *this->_pLoadingCanceled = true;
  }
}

SharedFuture<void>& TilesetMetadata::loadSchemaUri(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) {
  if (!this->_loadingFuture || this->_loadingSchemaUri != this->schemaUri) {
    this->_loadingSchemaUri = this->schemaUri;

    if (this->_pLoadingCanceled) {
      *this->_pLoadingCanceled = true;
      this->_pLoadingCanceled.reset();
    }

    if (!this->schemaUri) {
      this->_loadingFuture = asyncSystem.createResolvedFuture().share();
    } else {
      std::shared_ptr<bool> pLoadingCanceled = std::make_shared<bool>(false);
      this->_pLoadingCanceled = pLoadingCanceled;
      this->_loadingFuture =
          pAssetAccessor->get(asyncSystem, *this->schemaUri)
              .thenInMainThread([pLoadingCanceled, this, asyncSystem](
                                    std::shared_ptr<IAssetRequest>&& pRequest) {
                Promise<void> promise = asyncSystem.createPromise<void>();

                if (*pLoadingCanceled) {
                  promise.reject(std::runtime_error(fmt::format(
                      "Loading of schema URI {} was canceled.",
                      pRequest->url())));
                  return promise.getFuture();
                }

                const IAssetResponse* pResponse = pRequest->response();
                if (!pResponse) {
                  promise.reject(std::runtime_error(fmt::format(
                      "Did not receive a valid response for schema URI {}",
                      pRequest->url())));
                  return promise.getFuture();
                }

                uint16_t statusCode = pResponse->statusCode();
                if (statusCode != 0 &&
                    (statusCode < 200 || statusCode >= 300)) {
                  promise.reject(std::runtime_error(fmt::format(
                      "Received status code {} for schema URI {}.",
                      statusCode,
                      pRequest->url())));
                  return promise.getFuture();
                }

                SchemaReader reader;
                auto result = reader.readFromJson(pResponse->data());
                if (!result.value) {
                  std::string errors =
                      CesiumUtility::joinToString(result.errors, "\n - ");
                  if (!errors.empty()) {
                    errors = " Errors:\n - " + errors;
                  }
                  promise.reject(std::runtime_error(fmt::format(
                      "Error reading Schema from {}.{}",
                      pRequest->url(),
                      errors)));
                }

                this->schema = std::move(*result.value);

                promise.resolve();
                return promise.getFuture();
              })
              .share();
    }
  }

  return *this->_loadingFuture;
}

} // namespace Cesium3DTilesSelection
