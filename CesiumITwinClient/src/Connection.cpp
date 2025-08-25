#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/Promise.h>
#include <CesiumClientCommon/ErrorResponse.h>
#include <CesiumClientCommon/OAuth2PKCE.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumITwinClient/AuthenticationToken.h>
#include <CesiumITwinClient/CesiumCuratedContent.h>
#include <CesiumITwinClient/Connection.h>
#include <CesiumITwinClient/GeospatialFeatureCollection.h>
#include <CesiumITwinClient/IModel.h>
#include <CesiumITwinClient/IModelMeshExport.h>
#include <CesiumITwinClient/ITwin.h>
#include <CesiumITwinClient/ITwinRealityData.h>
#include <CesiumITwinClient/PagedList.h>
#include <CesiumITwinClient/Profile.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumUtility;
using namespace CesiumVectorData;

namespace CesiumITwinClient {
namespace {
const std::string ITWIN_AUTHORIZE_URL =
    "https://ims.bentley.com/connect/authorize";
const std::string ITWIN_TOKEN_URL = "https://ims.bentley.com/connect/token";

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

CesiumAsync::Future<CesiumUtility::Result<Connection>> Connection::authorize(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& friendlyApplicationName,
    const std::string& clientID,
    const std::string& redirectPath,
    const std::optional<int>& redirectPort,
    const std::vector<std::string>& scopes,
    std::function<void(const std::string&)>&& openUrlCallback) {
  CesiumClientCommon::OAuth2ClientOptions clientOptions{
      clientID,
      redirectPath,
      redirectPort,
      false};

  return CesiumClientCommon::OAuth2PKCE::authorize(
             asyncSystem,
             pAssetAccessor,
             friendlyApplicationName,
             clientOptions,
             scopes,
             std::move(openUrlCallback),
             ITWIN_TOKEN_URL,
             ITWIN_AUTHORIZE_URL)
      .thenImmediately(
          [asyncSystem, pAssetAccessor, clientOptions](
              const Result<CesiumClientCommon::OAuth2TokenResponse>& result)
              -> Result<Connection> {
            if (!result.value.has_value()) {
              return {result.errors};
            } else {
              Result<AuthenticationToken> authTokenResult =
                  AuthenticationToken::parse(result.value->accessToken);
              if (!authTokenResult.value.has_value() ||
                  !authTokenResult.value->isValid()) {
                return {authTokenResult.errors};
              } else {
                return Connection(
                    asyncSystem,
                    pAssetAccessor,
                    *authTokenResult.value,
                    result.value->refreshToken,
                    clientOptions);
              }
            }
          });
}

namespace {
const std::string ME_URL = "https://api.bentley.com/users/me";
}

CesiumAsync::Future<CesiumUtility::Result<UserProfile>> Connection::me() {
  return this->ensureValidToken().thenInWorkerThread(
      [asyncSystem = this->_asyncSystem,
       pAssetAccessor =
           this->_pAssetAccessor](const Result<std::string>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem.createResolvedFuture<Result<UserProfile>>(
              tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", *tokenResult.value},
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

namespace {
const std::string LIST_ITWINS_URL = "https://api.bentley.com/itwins/";
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwin>>>
Connection::itwins(const QueryParameters& params) {
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
           this->_pAssetAccessor](const Result<std::string>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem.createResolvedFuture<Result<PagedList<ITwin>>>(
              tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", *tokenResult.value},
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
                items.emplace_back(ITwin{
                    JsonHelpers::getStringOrDefault(item, "id", ""),
                    JsonHelpers::getStringOrDefault(item, "class", ""),
                    JsonHelpers::getStringOrDefault(item, "subClass", ""),
                    JsonHelpers::getStringOrDefault(item, "type", ""),
                    JsonHelpers::getStringOrDefault(item, "number", ""),
                    JsonHelpers::getStringOrDefault(item, "displayName", ""),
                    iTwinStatusFromString(
                        JsonHelpers::getStringOrDefault(item, "status", ""))});
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

namespace {
const std::string LIST_IMODELS_URL = "https://api.bentley.com/imodels/";
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<IModel>>>
Connection::imodels(const std::string& iTwinId, const QueryParameters& params) {
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
           this->_pAssetAccessor](const Result<std::string>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem.createResolvedFuture<Result<PagedList<IModel>>>(
              tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", *tokenResult.value},
            {"Accept", "application/vnd.bentley.itwin-platform.v2+json"},
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

                items.emplace_back(IModel{
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
                        northEast.latitude)});
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

namespace {
const std::string LIST_IMODEL_MESH_EXPORTS_URL =
    "https://api.bentley.com/mesh-export/";
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<IModelMeshExport>>>
Connection::meshExports(
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
           this->_pAssetAccessor](const Result<std::string>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem
              .createResolvedFuture<Result<PagedList<IModelMeshExport>>>(
                  tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", *tokenResult.value},
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

                items.emplace_back(IModelMeshExport{
                    JsonHelpers::getStringOrDefault(item, "id", ""),
                    JsonHelpers::getStringOrDefault(item, "displayName", ""),
                    iModelMeshExportStatusFromString(
                        JsonHelpers::getStringOrDefault(item, "status", "")),
                    exportType});
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

namespace {
const std::string LIST_ITWIN_REALITY_DATA_URL =
    "https://api.bentley.com/reality-management/reality-data/";
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwinRealityData>>>
Connection::realityData(
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
           this->_pAssetAccessor](const Result<std::string>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem
              .createResolvedFuture<Result<PagedList<ITwinRealityData>>>(
                  tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", *tokenResult.value},
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

                items.emplace_back(ITwinRealityData{
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
                    JsonHelpers::getBoolOrDefault(item, "authoring", false)});
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

namespace {
const std::string LIST_CCC_ENDPOINT_URL =
    "https://api.bentley.com/curated-content/cesium/";
}

using ITwinCCCListResponse = std::vector<CesiumCuratedContentAsset>;

CesiumAsync::Future<Result<ITwinCCCListResponse>>
Connection::cesiumCuratedContent() {
  const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
      {"Authorization", "Bearer " + this->_authenticationToken.getToken()},
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

            std::vector<CesiumCuratedContentAsset> items;
            items.reserve(itemsMember->value.Size());

            for (const auto& value : itemsMember->value.GetArray()) {
              items.emplace_back(CesiumCuratedContentAsset{
                  JsonHelpers::getUint64OrDefault(value, "id", 0),
                  cesiumCuratedContentTypeFromString(
                      JsonHelpers::getStringOrDefault(value, "type", "")),
                  JsonHelpers::getStringOrDefault(value, "name", ""),
                  JsonHelpers::getStringOrDefault(value, "description", ""),
                  JsonHelpers::getStringOrDefault(value, "attribution", ""),
                  cesiumCuratedContentStatusFromString(
                      JsonHelpers::getStringOrDefault(value, "status", ""))});
            }

            return Result<ITwinCCCListResponse>(std::move(items));
          });
}

CesiumAsync::Future<
    CesiumUtility::Result<std::vector<GeospatialFeatureCollection>>>
Connection::geospatialFeatureCollections(const std::string& iTwinId) {
  const std::string url = fmt::format(
      "https://api.bentley.com/geospatial-features/itwins/{}/ogc/collections",
      iTwinId);
  return this->ensureValidToken().thenInWorkerThread(
      [url,
       asyncSystem = this->_asyncSystem,
       pAssetAccessor =
           this->_pAssetAccessor](const Result<std::string>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem.createResolvedFuture<
              Result<std::vector<GeospatialFeatureCollection>>>(
              tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", *tokenResult.value},
            {"Accept", "application/vnd.bentley.itwin-platform.v1+json"}};

        return pAssetAccessor->get(asyncSystem, url, headers)
            .thenImmediately([](std::shared_ptr<IAssetRequest>&& request) {
              Result<rapidjson::Document> docResult = handleJsonResponse(
                  request,
                  "listing geospatial feature collections");
              if (!docResult.value) {
                return Result<std::vector<GeospatialFeatureCollection>>(
                    docResult.errors);
              }

              const rapidjson::Document& doc = *docResult.value;

              const auto& collectionsMember = doc.FindMember("collections");
              if (collectionsMember == doc.MemberEnd() ||
                  !collectionsMember->value.IsArray()) {
                return Result<std::vector<GeospatialFeatureCollection>>(
                    ErrorList::error(
                        "Collections result missing `collections` property."));
              }

              std::vector<GeospatialFeatureCollection> collections;
              collections.reserve(collectionsMember->value.Size());

              for (const auto& collection :
                   collectionsMember->value.GetArray()) {
                if (!collection.IsObject()) {
                  return Result<std::vector<GeospatialFeatureCollection>>(
                      ErrorList::error("All items in `collections` must be "
                                       "JSON objects - skipping "));
                }

                GeospatialFeatureCollection& collectionResult =
                    collections.emplace_back(GeospatialFeatureCollection{});

                // Parse extents first.
                const auto& extentMember = collection.FindMember("extent");
                if (extentMember == collection.MemberEnd() ||
                    !extentMember->value.IsObject()) {
                  return Result<std::vector<GeospatialFeatureCollection>>(
                      ErrorList::error(
                          "Collections result missing `extent` property."));
                }

                // Handle spatial extents
                const auto& spatialMember =
                    extentMember->value.FindMember("spatial");
                if (spatialMember == extentMember->value.MemberEnd() ||
                    !spatialMember->value.IsObject()) {
                  return Result<std::vector<GeospatialFeatureCollection>>(
                      ErrorList::error("Collections result missing "
                                       "`extent.spatial` property."));
                }

                const auto& bboxMember =
                    spatialMember->value.FindMember("bbox");
                if (bboxMember == spatialMember->value.MemberEnd() ||
                    !bboxMember->value.IsArray()) {
                  return Result<std::vector<GeospatialFeatureCollection>>(
                      ErrorList::error("Collections result missing "
                                       "`extent.spatial.bbox` property."));
                }

                collectionResult.extents.spatial.reserve(
                    bboxMember->value.Size());

                for (const auto& bboxCoords : bboxMember->value.GetArray()) {
                  std::optional<std::vector<double>> coords =
                      JsonHelpers::getDoubles(bboxCoords, -1);
                  if (!coords || (coords->size() != 4 && coords->size() != 6)) {
                    return Result<std::vector<GeospatialFeatureCollection>>(
                        ErrorList::error(
                            "Collections result `extent.spatial.bbox` member "
                            "must have either four or six components."));
                  }

                  collectionResult.extents.spatial.emplace_back(
                      (*coords)[0],
                      (*coords)[1],
                      coords->size() == 4 ? 0 : (*coords)[2],
                      coords->size() == 4 ? (*coords)[2] : (*coords)[3],
                      coords->size() == 4 ? (*coords)[3] : (*coords)[4],
                      coords->size() == 4 ? 0 : (*coords)[5]);
                }

                collectionResult.extents.coordinateReferenceSystem =
                    JsonHelpers::getStringOrDefault(
                        spatialMember->value,
                        "crs",
                        "");

                // Handle temporal extents
                const auto& temporalMember =
                    extentMember->value.FindMember("temporal");
                if (temporalMember != extentMember->value.MemberEnd() &&
                    temporalMember->value.IsObject()) {
                  const auto& intervalMember =
                      temporalMember->value.FindMember("interval");
                  if (intervalMember == temporalMember->value.MemberEnd() ||
                      !intervalMember->value.IsArray()) {
                    return Result<std::vector<GeospatialFeatureCollection>>(
                        ErrorList::error("Collections result missing "
                                         "`extent.temporal.interval` member."));
                  }

                  for (const auto& interval :
                       intervalMember->value.GetArray()) {
                    if (!interval.IsArray() || interval.Size() != 2) {
                      return Result<std::vector<GeospatialFeatureCollection>>(
                          ErrorList::error(
                              "Collections result `extent.temporal.interval` "
                              "member must be an array of two components."));
                    }

                    std::pair<std::string, std::string>& intervalPair =
                        collectionResult.extents.temporal.emplace_back();
                    if (interval[0].IsString()) {
                      intervalPair.first = interval[0].GetString();
                    } else if (interval[0].IsNull()) {
                      intervalPair.first = "";
                    } else {
                      return Result<std::vector<GeospatialFeatureCollection>>(
                          ErrorList::error(
                              "Collections result `extent.temporal.interval` "
                              "member arrays must contain only strings or null "
                              "values."));
                    }

                    if (interval[1].IsString()) {
                      intervalPair.second = interval[1].GetString();
                    } else if (interval[1].IsNull()) {
                      intervalPair.second = "";
                    } else {
                      return Result<std::vector<GeospatialFeatureCollection>>(
                          ErrorList::error(
                              "Collections result `extent.temporal.interval` "
                              "member arrays must contain only strings or null "
                              "values."));
                    }
                  }

                  collectionResult.extents.temporalReferenceSystem =
                      JsonHelpers::getStringOrDefault(
                          temporalMember->value,
                          "trs",
                          "");
                }

                collectionResult.id =
                    JsonHelpers::getStringOrDefault(collection, "id", "");
                collectionResult.title =
                    JsonHelpers::getStringOrDefault(collection, "title", "");
                collectionResult.description = JsonHelpers::getStringOrDefault(
                    collection,
                    "description",
                    "");
                collectionResult.crs =
                    JsonHelpers::getStrings(collection, "crs");
                collectionResult.storageCrs = JsonHelpers::getStringOrDefault(
                    collection,
                    "storageCrs",
                    "");
                std::string coordinateEpoch = JsonHelpers::getStringOrDefault(
                    collection,
                    "storageCrsCoordinateEpoch",
                    "");
                if (!coordinateEpoch.empty()) {
                  collectionResult.storageCrsCoordinateEpoch =
                      std::move(coordinateEpoch);
                }
              }

              return Result<std::vector<GeospatialFeatureCollection>>(
                  std::move(collections));
            });
      });
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<GeoJsonFeature>>>
Connection::geospatialFeatures(
    const std::string& iTwinId,
    const std::string& collectionId,
    uint32_t limit) {
  const uint32_t limitClamped = std::clamp<uint32_t>(limit, 1, 10000);
  const std::string url = fmt::format(
      "https://api.bentley.com/geospatial-features/itwins/{}/ogc/collections/"
      "{}/items?limit={}",
      iTwinId,
      collectionId,
      limitClamped);
  return this->listGeospatialFeatures(url);
}

CesiumAsync::Future<CesiumUtility::Result<PagedList<GeoJsonFeature>>>
Connection::listGeospatialFeatures(const std::string& url) {
  return this->ensureValidToken().thenInWorkerThread(
      [url,
       asyncSystem = this->_asyncSystem,
       pAssetAccessor =
           this->_pAssetAccessor](const Result<std::string>& tokenResult) {
        if (!tokenResult.value) {
          return asyncSystem
              .createResolvedFuture<Result<PagedList<GeoJsonFeature>>>(
                  tokenResult.errors);
        }

        const std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
            {"Authorization", *tokenResult.value},
            {"Accept", "application/vnd.bentley.itwin-platform.v1+json"}};

        return pAssetAccessor->get(asyncSystem, url, headers)
            .thenImmediately([](std::shared_ptr<IAssetRequest>&& request) {
              Result<rapidjson::Document> docResult =
                  handleJsonResponse(request, "listing geospatial features");
              if (!docResult.value) {
                return Result<PagedList<GeoJsonFeature>>(docResult.errors);
              }

              Result<GeoJsonDocument> geoJsonDocResult =
                  GeoJsonDocument::fromGeoJson(*docResult.value, {});
              if (!geoJsonDocResult.value) {
                return Result<PagedList<GeoJsonFeature>>(
                    geoJsonDocResult.errors);
              }

              GeoJsonFeatureCollection* pFeatureCollection =
                  geoJsonDocResult.value->rootObject
                      .getIf<GeoJsonFeatureCollection>();
              if (!pFeatureCollection) {
                return Result<PagedList<GeoJsonFeature>>(
                    ErrorList::error("Unable to obtain FeatureCollection from "
                                     "geospatial features response"));
              }

              std::vector<GeoJsonFeature> features;
              features.reserve(pFeatureCollection->features.size());
              for (GeoJsonObject& object : pFeatureCollection->features) {
                GeoJsonFeature* pFeature = object.getIf<GeoJsonFeature>();
                if (!pFeature) {
                  return Result<PagedList<GeoJsonFeature>>(
                      ErrorList::error(fmt::format(
                          "Expected only Feature objects to be in "
                          "FeatureCollection, found {}",
                          geoJsonObjectTypeToString(object.getType()))));
                }
                features.emplace_back(std::move(*pFeature));
              }

              return Result<PagedList<GeoJsonFeature>>(
                  PagedList<GeoJsonFeature>(
                      *docResult.value,
                      std::move(features),
                      [](Connection& connection, const std::string& url) {
                        return connection.listGeospatialFeatures(url);
                      }));
            });
      });
}

CesiumAsync::Future<CesiumUtility::Result<std::string>>
Connection::ensureValidToken() {
  if (this->_authenticationToken.isValid()) {
    return _asyncSystem.createResolvedFuture(
        Result<std::string>(this->_authenticationToken.getTokenHeader()));
  }

  if (!this->_refreshToken) {
    return _asyncSystem.createResolvedFuture(Result<std::string>(
        ErrorList::error("No valid auth token or refresh token.")));
  }

  return CesiumClientCommon::OAuth2PKCE::refresh(
             this->_asyncSystem,
             this->_pAssetAccessor,
             this->_clientOptions,
             ITWIN_TOKEN_URL,
             *this->_refreshToken)
      .thenInMainThread(
          [this](Result<CesiumClientCommon::OAuth2TokenResponse>&& response) {
            if (!response.value) {
              return Result<std::string>(response.errors);
            }

            Result<AuthenticationToken> tokenResult =
                AuthenticationToken::parse(response.value->accessToken);
            if (!tokenResult.value) {
              return Result<std::string>(tokenResult.errors);
            }

            this->_authenticationToken = std::move(*tokenResult.value);
            this->_refreshToken = std::move(response.value->refreshToken);

            return Result<std::string>(
                this->_authenticationToken.getTokenHeader());
          });
}

} // namespace CesiumITwinClient
