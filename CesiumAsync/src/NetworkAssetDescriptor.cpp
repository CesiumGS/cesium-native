#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Hash.h>
#include <CesiumUtility/Result.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace CesiumUtility;

std::size_t std::hash<CesiumAsync::NetworkAssetDescriptor>::operator()(
    const CesiumAsync::NetworkAssetDescriptor& key) const noexcept {
  std::hash<std::string> hash{};

  size_t result = hash(key.url);

  for (const CesiumAsync::IAssetAccessor::THeader& header : key.headers) {
    result = Hash::combine(result, hash(header.first));
    result = Hash::combine(result, hash(header.second));
  }

  return result;
}

namespace CesiumAsync {

bool NetworkAssetDescriptor::operator==(
    const NetworkAssetDescriptor& rhs) const noexcept {
  if (this->url != rhs.url || this->headers.size() != rhs.headers.size())
    return false;

  for (size_t i = 0; i < this->headers.size(); ++i) {
    if (this->headers[i].first != rhs.headers[i].first ||
        this->headers[i].second != rhs.headers[i].second)
      return false;
  }

  return true;
}

Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
NetworkAssetDescriptor::loadFromNetwork(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const {
  return pAssetAccessor->get(asyncSystem, this->url, this->headers);
}

Future<Result<std::vector<std::byte>>>
NetworkAssetDescriptor::loadBytesFromNetwork(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const {
  return this->loadFromNetwork(asyncSystem, pAssetAccessor)
      .thenInWorkerThread(
          [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest)
              -> Result<std::vector<std::byte>> {
            const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return ErrorList::error(
                  fmt::format("Request for {} failed.", pRequest->url()));
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              return ErrorList::error(fmt::format(
                  "Request for {} failed with code {}",
                  pRequest->url(),
                  pResponse->statusCode()));
            }

            return std::vector<std::byte>(
                pResponse->data().begin(),
                pResponse->data().end());
          });
}

} // namespace CesiumAsync
