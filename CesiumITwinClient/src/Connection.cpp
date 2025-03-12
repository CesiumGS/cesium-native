#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumUtility/ErrorList.h"
#include "CesiumUtility/Result.h"

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

#include <atomic>
#include <limits>
#include <string>

using namespace CesiumAsync;
using namespace CesiumUtility;

const std::string ITWIN_AUTHORIZE_URL =
    "https://ims.bentley.com/connect/authorize";
const std::string ITWIN_TOKEN_URL = "https://ims.bentley.com/connect/token";

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

CesiumGeospatial::Cartographic parsePoint(const rapidjson::Value& jsonValue) {
  double latitudeDegrees = 0, longitudeDegrees = 0;

  const auto& latitudeMember = jsonValue.FindMember("latitude");
  if (latitudeMember != jsonValue.MemberEnd() &&
      latitudeMember->value.IsDouble()) {
    latitudeDegrees = latitudeMember->value.GetDouble();
  }

  const auto& longitudeMember = jsonValue.FindMember("longitude");
  if (longitudeMember != jsonValue.MemberEnd() &&
      longitudeMember->value.IsDouble()) {
    longitudeDegrees = longitudeMember->value.GetDouble();
  }

  return CesiumGeospatial::Cartographic::fromDegrees(
      longitudeDegrees,
      latitudeDegrees);
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
    const std::optional<int>& redirectPort,
    const std::vector<std::string>& scopes,
    std::function<void(const std::string&)>&& openUrlCallback) {
  Promise<Connection> connectionPromise =
      asyncSystem.createPromise<Connection>();

  CesiumClientCommon::OAuth2ClientOptions clientOptions{
      clientID,
      redirectPath,
      redirectPort,
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

const std::string ME_URL = "https://api.bentley.com/users/me";

CesiumAsync::Future<CesiumUtility::Result<UserProfile>> Connection::me() {
  return this->ensureValidToken().thenInWorkerThread(
      [asyncSystem = this->_asyncSystem,
       pAssetAccessor =
           this->_pAssetAccessor](const Result<std::string_view>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem.createResolvedFuture<Result<UserProfile>>(
              tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", fmt::format("Bearer {}", *tokenResult.value)},
            {"Accept", "application/vnd.bentley.itwin-platform.v1+json"},
            {"Prefer", "return=representation"}};

        return pAssetAccessor->get(asyncSystem, ME_URL, headers)
            .thenImmediately([](std::shared_ptr<IAssetRequest>&& request) {
              Result<rapidjson::Document> docResult =
                  handleJsonResponse(request, "listing iModels");
              if (!docResult.value) {
                return Result<UserProfile>(docResult.errors);
              }

              const auto& userMember = docResult.value->FindMember("user");
              if (userMember == docResult.value->MemberEnd() ||
                  !userMember->value.IsObject()) {
                return Result<UserProfile>(
                    ErrorList::error("Missing `user` property in response."));
              }

              return Result<UserProfile>(UserProfile{
                  JsonHelpers::getStringOrDefault(userMember->value, "id", ""),
                  JsonHelpers::getStringOrDefault(
                      userMember->value,
                      "displayName",
                      ""),
                  JsonHelpers::getStringOrDefault(
                      userMember->value,
                      "givenName",
                      ""),
                  JsonHelpers::getStringOrDefault(
                      userMember->value,
                      "surname",
                      ""),
                  JsonHelpers::getStringOrDefault(
                      userMember->value,
                      "email",
                      "")});
            });
      });
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

const std::string LIST_IMODELS_URL = "https://api.bentley.com/imodels/";

CesiumAsync::Future<CesiumUtility::Result<PagedList<IModel>>>
Connection::listIModels(
    const std::string& iTwinId,
    const QueryParameters& params) {
  CesiumUtility::Uri uri(LIST_IMODELS_URL);
  CesiumUtility::UriQuery query(uri.getQuery());
  query.setValue("iTwinId", iTwinId);
  params.addToQuery(query);
  uri.setQuery(query.toQueryString());
  return this->listIModels(std::string(uri.toString()));
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<IModel>>>
Connection::listIModels(const std::string& url) {
  return this->ensureValidToken().thenInWorkerThread(
      [url,
       asyncSystem = this->_asyncSystem,
       pAssetAccessor =
           this->_pAssetAccessor](const Result<std::string_view>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem.createResolvedFuture<Result<PagedList<IModel>>>(
              tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", fmt::format("Bearer {}", *tokenResult.value)},
            {"Accept", "application/vnd.bentley.itwin-platform.v1+json"},
            {"Prefer", "return=representation"}};

        return pAssetAccessor->get(asyncSystem, url, headers)
            .thenImmediately([](std::shared_ptr<IAssetRequest>&& request) {
              Result<rapidjson::Document> docResult =
                  handleJsonResponse(request, "listing iModels");
              if (!docResult.value) {
                return Result<PagedList<IModel>>(docResult.errors);
              }

              const auto& itemsMember = docResult.value->FindMember("iModels");
              if (itemsMember == docResult.value->MemberEnd() ||
                  !itemsMember->value.IsArray()) {
                return Result<PagedList<IModel>>(ErrorList::error(
                    "List result missing `iModels` property."));
              }

              std::vector<IModel> items;
              items.reserve(itemsMember->value.Size());

              for (const auto& item : itemsMember->value.GetArray()) {
                CesiumGeospatial::Cartographic northEast(0, 0);
                CesiumGeospatial::Cartographic southWest(0, 0);

                // Parse extents rectangle from northEast and southWest
                // coordinates.
                const auto& extentMember = item.FindMember("extent");
                if (extentMember != item.MemberEnd() &&
                    extentMember->value.IsObject()) {
                  const auto& southWestMember =
                      extentMember->value.FindMember("southWest");
                  if (southWestMember != extentMember->value.MemberBegin() &&
                      southWestMember->value.IsObject()) {
                    southWest = parsePoint(southWestMember->value);
                  }

                  const auto& northEastMember =
                      extentMember->value.FindMember("northEast");
                  if (northEastMember != extentMember->value.MemberBegin() &&
                      northEastMember->value.IsObject()) {
                    northEast = parsePoint(northEastMember->value);
                  }
                }

                items.emplace_back(
                    JsonHelpers::getStringOrDefault(item, "id", ""),
                    JsonHelpers::getStringOrDefault(item, "displayName", ""),
                    JsonHelpers::getStringOrDefault(item, "name", ""),
                    JsonHelpers::getStringOrDefault(item, "description", ""),
                    iModelStateFromString(
                        JsonHelpers::getStringOrDefault(item, "state", "")),
                    CesiumGeospatial::GlobeRectangle(
                        southWest.longitude,
                        southWest.latitude,
                        northEast.longitude,
                        northEast.latitude));
              }

              return Result<PagedList<IModel>>(PagedList<IModel>(
                  *docResult.value,
                  std::move(items),
                  [](Connection& connection, const std::string& url) {
                    return connection.listIModels(url);
                  }));
            });
      });
}

const std::string LIST_IMODEL_MESH_EXPORTS_URL =
    "https://api.bentley.com/mesh-export/";

CesiumAsync::Future<CesiumUtility::Result<PagedList<IModelMeshExport>>>
Connection::listIModelMeshExports(
    const std::string& iModelId,
    const QueryParameters& params) {
  CesiumUtility::Uri uri(LIST_IMODEL_MESH_EXPORTS_URL);
  CesiumUtility::UriQuery query(uri.getQuery());
  query.setValue("iModelId", iModelId);
  params.addToQuery(query);
  uri.setQuery(query.toQueryString());
  return this->listIModelMeshExports(std::string(uri.toString()));
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<IModelMeshExport>>>
Connection::listIModelMeshExports(const std::string& url) {
  return this->ensureValidToken().thenInWorkerThread(
      [url,
       asyncSystem = this->_asyncSystem,
       pAssetAccessor =
           this->_pAssetAccessor](const Result<std::string_view>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem
              .createResolvedFuture<Result<PagedList<IModelMeshExport>>>(
                  tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", fmt::format("Bearer {}", *tokenResult.value)},
            {"Accept", "application/vnd.bentley.itwin-platform.v1+json"},
            {"Prefer", "return=representation"}};

        return pAssetAccessor->get(asyncSystem, url, headers)
            .thenImmediately([](std::shared_ptr<IAssetRequest>&& request) {
              Result<rapidjson::Document> docResult =
                  handleJsonResponse(request, "listing iModel mesh exports");
              if (!docResult.value) {
                return Result<PagedList<IModelMeshExport>>(docResult.errors);
              }

              const auto& itemsMember = docResult.value->FindMember("exports");
              if (itemsMember == docResult.value->MemberEnd() ||
                  !itemsMember->value.IsArray()) {
                return Result<PagedList<IModelMeshExport>>(ErrorList::error(
                    "List result missing `exports` property."));
              }

              std::vector<IModelMeshExport> items;
              items.reserve(itemsMember->value.Size());

              for (const auto& item : itemsMember->value.GetArray()) {
                IModelMeshExportType exportType = IModelMeshExportType::Unknown;

                const auto& requestMember = item.FindMember("request");
                if (requestMember != item.MemberEnd() &&
                    requestMember->value.IsObject()) {
                  exportType = iModelMeshExportTypeFromString(
                      JsonHelpers::getStringOrDefault(
                          requestMember->value,
                          "exportType",
                          ""));
                }

                items.emplace_back(
                    JsonHelpers::getStringOrDefault(item, "id", ""),
                    JsonHelpers::getStringOrDefault(item, "value", ""),
                    iModelMeshExportStatusFromString(
                        JsonHelpers::getStringOrDefault(item, "status", "")),
                    exportType);
              }

              return Result<PagedList<IModelMeshExport>>(
                  PagedList<IModelMeshExport>(
                      *docResult.value,
                      std::move(items),
                      [](Connection& connection, const std::string& url) {
                        return connection.listIModelMeshExports(url);
                      }));
            });
      });
}

const std::string LIST_ITWIN_REALITY_DATA_URL =
    "https://api.bentley.com/reality-management/reality-data/";

CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwinRealityData>>>
Connection::listITwinRealityData(
    const std::string& iTwinId,
    const QueryParameters& params) {
  CesiumUtility::Uri uri(LIST_ITWIN_REALITY_DATA_URL);
  CesiumUtility::UriQuery query(uri.getQuery());
  query.setValue("iTwinId", iTwinId);
  params.addToQuery(query);
  uri.setQuery(query.toQueryString());
  return this->listITwinRealityData(std::string(uri.toString()));
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwinRealityData>>>
Connection::listITwinRealityData(const std::string& url) {
  return this->ensureValidToken().thenInWorkerThread(
      [url,
       asyncSystem = this->_asyncSystem,
       pAssetAccessor =
           this->_pAssetAccessor](const Result<std::string_view>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem
              .createResolvedFuture<Result<PagedList<ITwinRealityData>>>(
                  tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", fmt::format("Bearer {}", *tokenResult.value)},
            {"Accept", "application/vnd.bentley.itwin-platform.v1+json"},
            {"Prefer", "return=representation"}};

        return pAssetAccessor->get(asyncSystem, url, headers)
            .thenImmediately([](std::shared_ptr<IAssetRequest>&& request) {
              Result<rapidjson::Document> docResult =
                  handleJsonResponse(request, "listing iTwin reality data");
              if (!docResult.value) {
                return Result<PagedList<ITwinRealityData>>(docResult.errors);
              }

              const auto& itemsMember =
                  docResult.value->FindMember("realityData");
              if (itemsMember == docResult.value->MemberEnd() ||
                  !itemsMember->value.IsArray()) {
                return Result<PagedList<ITwinRealityData>>(ErrorList::error(
                    "List result missing `realityData` property."));
              }

              std::vector<ITwinRealityData> items;
              items.reserve(itemsMember->value.Size());

              for (const auto& item : itemsMember->value.GetArray()) {
                CesiumGeospatial::Cartographic northEast(0, 0);
                CesiumGeospatial::Cartographic southWest(0, 0);

                // Parse extents rectangle from northEast and southWest
                // coordinates.
                const auto& extentMember = item.FindMember("extent");
                if (extentMember != item.MemberEnd() &&
                    extentMember->value.IsObject()) {
                  const auto& southWestMember =
                      extentMember->value.FindMember("southWest");
                  if (southWestMember != extentMember->value.MemberBegin() &&
                      southWestMember->value.IsObject()) {
                    southWest = parsePoint(southWestMember->value);
                  }

                  const auto& northEastMember =
                      extentMember->value.FindMember("northEast");
                  if (northEastMember != extentMember->value.MemberBegin() &&
                      northEastMember->value.IsObject()) {
                    northEast = parsePoint(northEastMember->value);
                  }
                }

                items.emplace_back(
                    JsonHelpers::getStringOrDefault(item, "id", ""),
                    JsonHelpers::getStringOrDefault(item, "displayName", ""),
                    JsonHelpers::getStringOrDefault(item, "description", ""),
                    iTwinRealityDataClassificationFromString(
                        JsonHelpers::getStringOrDefault(
                            item,
                            "classification",
                            "")),
                    JsonHelpers::getStringOrDefault(item, "type", ""),
                    CesiumGeospatial::GlobeRectangle(
                        southWest.longitude,
                        southWest.latitude,
                        northEast.longitude,
                        northEast.latitude),
                    JsonHelpers::getBoolOrDefault(item, "authoring", false));
              }

              return Result<PagedList<ITwinRealityData>>(
                  PagedList<ITwinRealityData>(
                      *docResult.value,
                      std::move(items),
                      [](Connection& connection, const std::string& url) {
                        return connection.listITwinRealityData(url);
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
      {"Authorization", "Bearer " + this->_accessToken.getToken()},
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
  if (this->_accessToken.isValid()) {
    return _asyncSystem.createResolvedFuture(
        Result<std::string_view>(this->_accessToken.getToken()));
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

            this->_accessToken = std::move(*tokenResult.value);
            this->_refreshToken = std::move(response.value->refreshToken);

            return Result<std::string_view>(this->_accessToken.getToken());
          });
}

namespace {
struct ProgressTracker {
public:
  ProgressTracker(std::function<void(
                      const std::atomic<int32_t>&,
                      const std::atomic<int32_t>&)>&& statusCallback)
      : _finishedCount(0),
        _totalCount(0),
        _callback(std::move(statusCallback)) {}

  void incrementFinished() {
    _finishedCount++;
    _callback(_finishedCount, _totalCount);
  }

  void incrementTotal() {
    _totalCount++;
    _callback(_finishedCount, _totalCount);
  }

private:
  std::atomic<int32_t> _finishedCount;
  std::atomic<int32_t> _totalCount;
  std::function<void(const std::atomic<int32_t>&, const std::atomic<int32_t>&)>
      _callback;
};

Result<std::vector<ITwinResource>>
flattenResults(std::vector<Result<std::vector<ITwinResource>>>&& results) {
  ErrorList errorList;
  std::vector<ITwinResource> resources;
  for (Result<std::vector<ITwinResource>>& result : results) {
    errorList.merge(result.errors);
    if (result.value) {
      resources.insert(
          resources.end(),
          result.value->begin(),
          result.value->end());
    }
  }

  if (errorList.hasErrors() && resources.empty()) {
    return {errorList};
  }

  return {std::move(resources), errorList};
}

CesiumAsync::Future<CesiumUtility::Result<std::vector<ITwinResource>>>
fetchCCCResources(
    Connection& connection,
    std::shared_ptr<ProgressTracker>& progress) {
  progress->incrementTotal();
  return connection.listCesiumCuratedContent().thenInWorkerThread(
      [progress](
          CesiumUtility::Result<std::vector<ITwinCesiumCuratedContentItem>>&&
              result) -> Result<std::vector<ITwinResource>> {
        progress->incrementFinished();
        if (!result.value) {
          return {result.errors};
        }

        std::vector<ITwinResource> cccResources;
        cccResources.reserve(result.value->size());

        for (const ITwinCesiumCuratedContentItem& cccItem : *result.value) {
          ResourceType type;
          if (cccItem.type == ITwinCesiumCuratedContentType::Cesium3DTiles) {
            type = ResourceType::Tileset;
          } else if (cccItem.type == ITwinCesiumCuratedContentType::Imagery) {
            type = ResourceType::Imagery;
          } else if (cccItem.type == ITwinCesiumCuratedContentType::Terrain) {
            type = ResourceType::Terrain;
          } else {
            continue;
          }

          cccResources.emplace_back(
              std::to_string(cccItem.id),
              std::nullopt,
              ResourceSource::CesiumCuratedContent,
              cccItem.name,
              type);
        }

        return cccResources;
      });
}

CesiumAsync::Future<CesiumUtility::Result<std::vector<ITwinResource>>>
fetchIModelMeshExports(
    Connection& connection,
    std::shared_ptr<ProgressTracker>& progress,
    const std::string& iModelId) {
  progress->incrementTotal();
  return connection
      .listIModelMeshExports(
          iModelId,
          QueryParameters{std::nullopt, std::nullopt, 1000, std::nullopt})
      .thenInWorkerThread(
          [iModelId, progress](Result<PagedList<IModelMeshExport>>&& result) {
            progress->incrementFinished();
            if (!result.value) {
              return Result<std::vector<ITwinResource>>(result.errors);
            }

            std::vector<ITwinResource> resources;
            resources.reserve(result.value->size());

            for (const IModelMeshExport& exp : *result.value) {
              if (exp.exportType != IModelMeshExportType::Cesium &&
                  exp.exportType != IModelMeshExportType::Cesium3DTiles) {
                continue;
              }

              resources.emplace_back(
                  exp.id,
                  iModelId,
                  ResourceSource::MeshExport,
                  exp.displayName,
                  ResourceType::Tileset);
            }

            return Result<std::vector<ITwinResource>>(std::move(resources));
          });
}

CesiumAsync::Future<CesiumUtility::Result<std::vector<ITwinResource>>>
fetchIModelResources(
    AsyncSystem& asyncSystem,
    Connection& connection,
    std::shared_ptr<ProgressTracker>& progress,
    const std::string& iTwinId) {
  progress->incrementTotal();
  return connection
      .listIModels(
          iTwinId,
          QueryParameters{std::nullopt, std::nullopt, 1000, std::nullopt})
      .thenInWorkerThread([asyncSystem, progress, connection](
                              Result<PagedList<IModel>>&& result) mutable {
        progress->incrementFinished();
        if (!result.value) {
          return asyncSystem
              .createResolvedFuture<Result<std::vector<ITwinResource>>>(
                  {result.errors});
        }

        std::vector<CesiumAsync::Future<
            CesiumUtility::Result<std::vector<ITwinResource>>>>
            resourcesFutures;
        for (const IModel& iModel : *result.value) {
          resourcesFutures.emplace_back(
              fetchIModelMeshExports(connection, progress, iModel.id));
        }

        return asyncSystem.all(std::move(resourcesFutures))
            .thenImmediately(flattenResults);
      });
}

CesiumAsync::Future<CesiumUtility::Result<std::vector<ITwinResource>>>
fetchRealityDataResources(
    Connection& connection,
    std::shared_ptr<ProgressTracker>& progress,
    const std::string& iTwinId) {
  progress->incrementTotal();
  return connection
      .listITwinRealityData(
          iTwinId,
          QueryParameters{std::nullopt, std::nullopt, 1000, std::nullopt})
      .thenInWorkerThread(
          [progress, iTwinId](Result<PagedList<ITwinRealityData>>&& result) {
            progress->incrementFinished();
            if (!result.value) {
              return Result<std::vector<ITwinResource>>{result.errors};
            }
            std::vector<ITwinResource> resources;
            resources.reserve(result.value->size());

            for (const ITwinRealityData& data : *result.value) {
              ResourceType type;
              if (data.type == "Cesium3DTiles" ||
                  data.type == "RealityMesh3DTiles" || data.type == "PNTS") {
                type = ResourceType::Tileset;
              } else if (data.type == "Terrain3DTiles") {
                type = ResourceType::Terrain;
              } else {
                continue;
              }

              resources.emplace_back(
                  data.id,
                  iTwinId,
                  ResourceSource::RealityData,
                  data.displayName,
                  type);
            }

            return Result<std::vector<ITwinResource>>{resources};
          });
}

CesiumAsync::Future<CesiumUtility::Result<std::vector<ITwinResource>>>
fetchITwinResources(
    AsyncSystem& asyncSystem,
    Connection& connection,
    std::shared_ptr<ProgressTracker>& progress) {
  progress->incrementTotal();
  // TODO: it's *possible* they have more than 1,000 total iTwins, or 1,000
  // total iModels per iTwin, etc. We should really enumerate each page...
  return connection
      .listITwins(
          QueryParameters{std::nullopt, std::nullopt, 1000, std::nullopt})
      .thenInWorkerThread([progress, connection, asyncSystem](
                              Result<PagedList<ITwin>>&& result) mutable {
        progress->incrementFinished();
        if (!result.value) {
          return asyncSystem
              .createResolvedFuture<Result<std::vector<ITwinResource>>>(
                  {result.errors});
        }

        std::vector<CesiumAsync::Future<
            CesiumUtility::Result<std::vector<ITwinResource>>>>
            resourcesFutures;
        for (const ITwin& iTwin : *result.value) {
          resourcesFutures.emplace_back(
              fetchRealityDataResources(connection, progress, iTwin.id));
          resourcesFutures.emplace_back(fetchIModelResources(
              asyncSystem,
              connection,
              progress,
              iTwin.id));
        }

        return asyncSystem.all(std::move(resourcesFutures))
            .thenImmediately(flattenResults);
      });
}
} // namespace

CesiumAsync::Future<CesiumUtility::Result<std::vector<ITwinResource>>>
Connection::listAllAvailableResources(
    std::function<
        void(const std::atomic<int32_t>&, const std::atomic<int32_t>&)>&&
        statusCallback) {
  std::shared_ptr<ProgressTracker> progress =
      std::make_shared<ProgressTracker>(statusCallback);

  std::vector<
      CesiumAsync::Future<CesiumUtility::Result<std::vector<ITwinResource>>>>
      futures;

  futures.emplace_back(fetchCCCResources(*this, progress));
  futures.emplace_back(
      fetchITwinResources(this->_asyncSystem, *this, progress));

  return this->_asyncSystem.all(std::move(futures))
      .thenImmediately(flattenResults);
}
} // namespace CesiumITwinClient