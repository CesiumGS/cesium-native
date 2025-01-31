#include "parseLinkHeader.h"

#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumIonClient/ApplicationData.h>
#include <CesiumIonClient/Assets.h>
#include <CesiumIonClient/Defaults.h>
#include <CesiumIonClient/Geocoder.h>
#include <CesiumIonClient/Profile.h>
#include <CesiumIonClient/Response.h>
#include <CesiumIonClient/Token.h>
#include <CesiumIonClient/TokenList.h>
#include <CesiumUtility/Uri.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumIonClient {

template <typename T> Response<T>::Response() = default;

template <typename T>
Response<T>::Response(
    T&& value_,
    uint16_t httpStatusCode_,
    const std::string& errorCode_,
    const std::string& errorMessage_)
    : value(value_),
      httpStatusCode(httpStatusCode_),
      errorCode(errorCode_),
      errorMessage(errorMessage_),
      nextPageUrl(),
      previousPageUrl() {}

template <typename T>
Response<T>::Response(
    uint16_t httpStatusCode_,
    const std::string& errorCode_,
    const std::string& errorMessage_)
    : value(),
      httpStatusCode(httpStatusCode_),
      errorCode(errorCode_),
      errorMessage(errorMessage_),
      nextPageUrl(),
      previousPageUrl() {}

template <typename T>
Response<T>::Response(
    const std::shared_ptr<CesiumAsync::IAssetRequest>& pRequest,
    T&& value_)
    : value(value_),
      httpStatusCode(pRequest->response()->statusCode()),
      errorCode(),
      errorMessage(),
      nextPageUrl(),
      previousPageUrl() {
  const HttpHeaders& headers = pRequest->response()->headers();
  auto it = headers.find("link");
  if (it == headers.end()) {
    return;
  }

  const std::string& linkHeader = it->second;
  std::vector<Link> links = parseLinkHeader(linkHeader);
  for (const Link& link : links) {
    if (link.rel == "next") {
      this->nextPageUrl = Uri::resolve(pRequest->url(), link.url);
    } else if (link.rel == "prev") {
      this->previousPageUrl = Uri::resolve(pRequest->url(), link.url);
    }
  }
}

// Explicit instantiations
template struct Response<Asset>;
template struct Response<Assets>;
template struct Response<Defaults>;
template struct Response<NoValue>;
template struct Response<Profile>;
template struct Response<Token>;
template struct Response<TokenList>;
template struct Response<ApplicationData>;
template struct Response<GeocoderResult>;

} // namespace CesiumIonClient
