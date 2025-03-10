#include "CesiumAsync/IAssetRequest.h"
#include "CesiumUtility/ErrorList.h"

#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/Promise.h>
#include <CesiumClientCommon/ErrorResponse.h>
#include <CesiumClientCommon/OAuth2PKE.h>
#include <CesiumITwinClient/AuthToken.h>
#include <CesiumITwinClient/Connection.h>
#include <CesiumITwinClient/PagedList.h>
#include <CesiumITwinClient/Resources.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <limits>
#include <string>

using namespace CesiumAsync;
using namespace CesiumUtility;

const std::string ITWIN_AUTHORIZE_URL =
    "https://ims.bentley.com/connect/authorize";
const std::string ITWIN_TOKEN_URL = "https://ims.bentley.com/connect/token";
const int REDIRECT_URI_PORT = 5081;

namespace CesiumITwinClient {
namespace {
Result<rapidjson::Document> handleJsonResponse(
    std::shared_ptr<CesiumAsync::IAssetRequest>& pRequest,
    const std::string& operation) {
  const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
  if (!pResponse) {
    return Result<rapidjson::Document>(
        ErrorList::error("The server did not return a response."));
  }

  if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
    std::string error, errorDesc;
    if (CesiumClientCommon::parseErrorResponse(
            pResponse->data(),
            error,
            errorDesc)) {
      return Result<rapidjson::Document>(ErrorList::error(fmt::format(
          "Received error '{}' while {}: {}",
          error,
          operation,
          errorDesc)));
    }

    return Result<rapidjson::Document>(ErrorList::error(fmt::format(
        "The server returned an error code: {}",
        pResponse->statusCode())));
  }

  rapidjson::Document d;
  d.Parse(
      reinterpret_cast<const char*>(pResponse->data().data()),
      pResponse->data().size());
  if (d.HasParseError()) {
    return Result<rapidjson::Document>(ErrorList::error(fmt::format(
        "Failed to parse JSON response: {}",
        rapidjson::GetParseError_En(d.GetParseError()))));
  } else if (!d.IsObject()) {
    return Result<rapidjson::Document>(
        ErrorList::error("No JSON object contained in response."));
  }

  return Result<rapidjson::Document>(std::move(d));
}
} // namespace

void QueryParameters::addToQuery(CesiumUtility::UriQuery& query) const {
  if (this->search) {
    query.setValue("$search", *this->search);
  }
  if (this->orderBy) {
    query.setValue("$orderBy", *this->orderBy);
  }
  if (this->top) {
    query.setValue("$top", std::to_string(*this->top));
  }
  if (this->skip) {
    query.setValue("$skip", std::to_string(*this->skip));
  }
}

void QueryParameters::addToUri(CesiumUtility::Uri& uri) const {
  CesiumUtility::UriQuery query(uri.getQuery());
  addToQuery(query);
  uri.setQuery(query.toQueryString());
}

CesiumAsync::Future<Connection> Connection::authorize(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& friendlyApplicationName,
    const std::string& clientID,
    const std::string& redirectPath,
    const std::vector<std::string>& scopes,
    std::function<void(const std::string&)>&& openUrlCallback) {
  Promise<Connection> connectionPromise =
      asyncSystem.createPromise<Connection>();

  CesiumClientCommon::OAuth2ClientOptions clientOptions{
      clientID,
      redirectPath,
      REDIRECT_URI_PORT,
      false};

  CesiumClientCommon::OAuth2PKE::authorize(
      asyncSystem,
      pAssetAccessor,
      friendlyApplicationName,
      clientOptions,
      scopes,
      std::move(openUrlCallback),
      ITWIN_TOKEN_URL,
      ITWIN_AUTHORIZE_URL)
      .thenImmediately(
          [asyncSystem, pAssetAccessor, connectionPromise, clientOptions](
              const Result<CesiumClientCommon::OAuth2TokenResponse>& result) {
            if (!result.value.has_value()) {
              connectionPromise.reject(std::runtime_error(fmt::format(
                  "Failed to complete authorization: {}",
                  joinToString(result.errors.errors, ", "))));
            } else {
              Result<AuthToken> authTokenResult =
                  AuthToken::parse(result.value->accessToken);
              if (!authTokenResult.value.has_value() ||
                  !authTokenResult.value->isValid()) {
                connectionPromise.reject(std::runtime_error(fmt::format(
                    "Invalid auth token obtained: {}",
                    joinToString(authTokenResult.errors.errors, ", "))));
              } else {
                connectionPromise.resolve(Connection(
                    asyncSystem,
                    pAssetAccessor,
                    *authTokenResult.value,
                    result.value->refreshToken,
                    clientOptions));
              }
            }
          });

  return connectionPromise.getFuture();
}

const std::string LIST_ITWINS_URL = "https://api.bentley.com/itwins/";

CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwin>>>
Connection::listITwins(const QueryParameters& params) {
  CesiumUtility::Uri uri(LIST_ITWINS_URL);
  params.addToUri(uri);
  return this->listITwins(std::string(uri.toString()));
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwin>>>
Connection::listITwins(const std::string& url) {
  return this->ensureValidToken().thenInWorkerThread(
      [url,
       asyncSystem = this->_asyncSystem,
       pAssetAccessor =
           this->_pAssetAccessor](const Result<std::string_view>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem.createResolvedFuture<Result<PagedList<ITwin>>>(
              tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", fmt::format("Bearer {}", *tokenResult.value)},
            {"Accept", "application/vnd.bentley.itwin-platform.v1+json"},
            {"Prefer", "return=representation"}};

        return pAssetAccessor->get(asyncSystem, url, headers)
            .thenImmediately([](std::shared_ptr<IAssetRequest>&& request) {
              Result<rapidjson::Document> docResult =
                  handleJsonResponse(request, "listing iTwins");
              if (!docResult.value) {
                return Result<PagedList<ITwin>>(docResult.errors);
              }

              const auto& itemsMember = docResult.value->FindMember("iTwins");
              if (itemsMember == docResult.value->MemberEnd() ||
                  !itemsMember->value.IsArray()) {
                return Result<PagedList<ITwin>>(
                    ErrorList::error("List result missing `iTwins` property."));
              }

              std::vector<ITwin> items;
              items.reserve(itemsMember->value.Size());

              for (const auto& item : itemsMember->value.GetArray()) {
                items.emplace_back(
                    JsonHelpers::getStringOrDefault(item, "id", ""),
                    JsonHelpers::getStringOrDefault(item, "class", ""),
                    JsonHelpers::getStringOrDefault(item, "subClass", ""),
                    JsonHelpers::getStringOrDefault(item, "type", ""),
                    JsonHelpers::getStringOrDefault(item, "number", ""),
                    JsonHelpers::getStringOrDefault(item, "displayName", ""),
                    iTwinStatusFromString(
                        JsonHelpers::getStringOrDefault(item, "status", "")));
              }

              return Result<PagedList<ITwin>>(PagedList<ITwin>(
                  *docResult.value,
                  std::move(items),
                  [](Connection& connection, const std::string& url) {
                    return connection.listITwins(url);
                  }));
            });
      });
}

const std::string LIST_CCC_ENDPOINT_URL =
    "https://api.bentley.com/curated-content/cesium/";
using ITwinCCCListResponse = std::vector<ITwinCesiumCuratedContentItem>;

CesiumAsync::Future<Result<ITwinCCCListResponse>>
Connection::listCesiumCuratedContent() {
  const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
      {"Authorization", "Bearer " + this->_authToken.getToken()},
      {"Accept", "application/vnd.bentley.itwin-platform.v1+json"}};
  return this->_pAssetAccessor
      ->get(this->_asyncSystem, LIST_CCC_ENDPOINT_URL, headers)
      .thenInWorkerThread(
          [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
            Result<rapidjson::Document> docResult =
                handleJsonResponse(pRequest, "listing Cesium curated content");
            if (!docResult.value) {
              return Result<ITwinCCCListResponse>(docResult.errors);
            }

            const rapidjson::Document& doc = *docResult.value;

            const auto& itemsMember = doc.FindMember("items");
            if (itemsMember == doc.MemberEnd() ||
                !itemsMember->value.IsArray()) {
              return Result<ITwinCCCListResponse>(
                  ErrorList::error("Can't find list of items in Cesium curated "
                                   "content list response."));
            }

            std::vector<ITwinCesiumCuratedContentItem> items;
            items.reserve(itemsMember->value.Size());

            for (const auto& value : itemsMember->value.GetArray()) {
              items.emplace_back(
                  JsonHelpers::getUint64OrDefault(value, "id", 0),
                  cesiumCuratedContentTypeFromString(
                      JsonHelpers::getStringOrDefault(value, "type", "")),
                  JsonHelpers::getStringOrDefault(value, "name", ""),
                  JsonHelpers::getStringOrDefault(value, "description", ""),
                  JsonHelpers::getStringOrDefault(value, "attribution", ""),
                  cesiumCuratedContentStatusFromString(
                      JsonHelpers::getStringOrDefault(value, "status", "")));
            }

            return Result<ITwinCCCListResponse>(std::move(items));
          });
}
CesiumAsync::Future<CesiumUtility::Result<std::string_view>>
Connection::ensureValidToken() {
  if (this->_authToken.isValid()) {
    return _asyncSystem.createResolvedFuture(
        Result<std::string_view>(this->_authToken.getToken()));
  }

  if (!this->_refreshToken) {
    return _asyncSystem.createResolvedFuture(Result<std::string_view>(
        ErrorList::error("No valid auth token or refresh token.")));
  }

  return CesiumClientCommon::OAuth2PKE::refresh(
             this->_asyncSystem,
             this->_pAssetAccessor,
             this->_clientOptions,
             ITWIN_TOKEN_URL,
             *this->_refreshToken)
      .thenInMainThread(
          [this](Result<CesiumClientCommon::OAuth2TokenResponse>&& response) {
            if (!response.value) {
              return Result<std::string_view>(response.errors);
            }

            Result<AuthToken> tokenResult =
                AuthToken::parse(response.value->accessToken);
            if (!tokenResult.value) {
              return Result<std::string_view>(tokenResult.errors);
            }

            this->_authToken = std::move(*tokenResult.value);
            this->_refreshToken = std::move(response.value->refreshToken);

            return Result<std::string_view>(this->_authToken.getToken());
          });
}
} // namespace CesiumITwinClient