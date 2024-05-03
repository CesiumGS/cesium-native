#include "CesiumIonClient/Response.h"

#include "CesiumIonClient/ApplicationData.h"
#include "CesiumIonClient/Assets.h"
#include "CesiumIonClient/Defaults.h"
#include "CesiumIonClient/Profile.h"
#include "CesiumIonClient/TokenList.h"
#include "parseLinkHeader.h"

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Uri.h>

#include <cassert>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumIonClient {

template <typename T> Response<T>::Response() {}

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

} // namespace CesiumIonClient
