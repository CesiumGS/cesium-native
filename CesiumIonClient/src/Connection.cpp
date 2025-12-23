#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/Promise.h>
#include <CesiumClientCommon/JwtTokenUtility.h>
#include <CesiumClientCommon/OAuth2PKCE.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumIonClient/ApplicationData.h>
#include <CesiumIonClient/Assets.h>
#include <CesiumIonClient/Connection.h>
#include <CesiumIonClient/Defaults.h>
#include <CesiumIonClient/Geocoder.h>
#include <CesiumIonClient/LoginToken.h>
#include <CesiumIonClient/Profile.h>
#include <CesiumIonClient/Response.h>
#include <CesiumIonClient/Token.h>
#include <CesiumIonClient/TokenList.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/pointer.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumIonClient;
using namespace CesiumUtility;

/*static*/ CesiumAsync::Future<Result<Connection>> Connection::authorize(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& friendlyApplicationName,
    int64_t clientID,
    const std::string& redirectPath,
    const std::vector<std::string>& scopes,
    std::function<void(const std::string&)>&& openUrlCallback,
    const CesiumIonClient::ApplicationData& appData,
    const std::string& ionApiUrl,
    const std::string& ionAuthorizeUrl) {
  const std::string tokenUrl = Uri::resolve(ionApiUrl, "oauth/token");

  return CesiumClientCommon::OAuth2PKCE::authorize(
             asyncSystem,
             pAssetAccessor,
             friendlyApplicationName,
             CesiumClientCommon::OAuth2ClientOptions{
                 std::to_string(clientID),
                 redirectPath,
                 std::nullopt,
                 true},
             scopes,
             std::move(openUrlCallback),
             tokenUrl,
             ionAuthorizeUrl)
      .thenImmediately(
          [asyncSystem,
           pAssetAccessor,
           ionApiUrl,
           appData,
           clientID,
           redirectPath](
              const Result<CesiumClientCommon::OAuth2TokenResponse>& result) {
            ErrorList combinedList =
                ErrorList::error("Failed to complete authorization:");
            if (!result.value.has_value()) {
              combinedList.merge(result.errors);
              return Result<Connection>(combinedList);
            } else {
              Result<LoginToken> tokenResult =
                  LoginToken::parse(result.value->accessToken);
              if (!tokenResult.value) {
                combinedList.merge(tokenResult.errors);
                return Result<Connection>(combinedList);
              }
              return Result<Connection>(Connection(
                  asyncSystem,
                  pAssetAccessor,
                  *tokenResult.value,
                  result.value->refreshToken.value_or(""),
                  clientID,
                  redirectPath,
                  appData,
                  ionApiUrl));
            }
          });
}

Connection::Connection(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const CesiumIonClient::LoginToken& accessToken,
    const std::string& refreshToken,
    int64_t clientId,
    const std::string& redirectPath,
    const CesiumIonClient::ApplicationData& appData,
    const std::string& apiUrl)
    : _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _apiUrl(apiUrl),
      _appData(appData),
      _clientId(clientId),
      _redirectPath(redirectPath),
      _pTokenDetails(
          std::make_shared<TokenDetails>(accessToken, refreshToken)) {}

Connection::Connection(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& accessToken,
    const CesiumIonClient::ApplicationData& appData,
    const std::string& apiUrl)
    : Connection(
          asyncSystem,
          pAssetAccessor,
          LoginToken(accessToken, -1),
          "",
          0,
          "",
          appData,
          apiUrl) {}

Connection::Connection(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const CesiumIonClient::ApplicationData& appData,
    const std::string& apiUrl)
    : Connection(
          asyncSystem,
          pAssetAccessor,
          "",
          appData,
          apiUrl) {}

const CesiumAsync::AsyncSystem& Connection::getAsyncSystem() const noexcept {
  return this->_asyncSystem;
}

const std::shared_ptr<CesiumAsync::IAssetAccessor>&
Connection::getAssetAccessor() const noexcept {
  return this->_pAssetAccessor;
}

const std::string& Connection::getAccessToken() const noexcept {
  return this->_pTokenDetails->accessToken.getToken();
}

const std::string& Connection::getRefreshToken() const noexcept {
  return this->_pTokenDetails->refreshToken;
}

const std::string& Connection::getApiUrl() const noexcept {
  return this->_apiUrl;
}

namespace {

template <typename T> Response<T> createEmptyResponse() {
  return Response<T>{0, "NoResponse", "The server did not return a response."};
}

template <typename T>
Response<T> createErrorResponse(const IAssetResponse* pResponse) {
  return Response<T>{
      pResponse->statusCode(),
      std::to_string(pResponse->statusCode()),
      "Received response code " + std::to_string(pResponse->statusCode())};
}

template <typename T>
Response<T> createJsonErrorResponse(
    const IAssetResponse* pResponse,
    const rapidjson::Document& d) {
  return Response<T>{
      pResponse->statusCode(),
      "ParseError",
      std::string("Failed to parse JSON response: ") +
          rapidjson::GetParseError_En(d.GetParseError())};
}

template <typename T>
Response<T> createJsonTypeResponse(
    const IAssetResponse* pResponse,
    const std::string& expectedType) {
  return Response<T>{
      pResponse->statusCode(),
      "ParseError",
      "Response is not a JSON " + expectedType + "."};
}

template <typename T> Response<T> createNoMorePagesResponse() {
  return Response<T>{
      0,
      "NoMorePages",
      "There are no more pages after the current one."};
}

template <typename T>
Response<T> createNoValidTokenResponse(Result<std::string>&& result) {
  return Response<T>(
      403,
      "NoValidToken",
      result.errors.format("Failed to get valid access token:"));
}

bool parseJsonObject(const IAssetResponse* pResponse, rapidjson::Document& d) {
  d.Parse(
      reinterpret_cast<const char*>(pResponse->data().data()),
      pResponse->data().size());
  return !d.HasParseError();
}

} // namespace

CesiumAsync::Future<Response<Profile>> Connection::me() const {
  // /v1/me endpoint doesn't exist when ion is running in single user mode
  if (this->_appData.authenticationMode == AuthenticationMode::SingleUser) {
    Profile profile;
    profile.id = 0;
    profile.username = "ion-user";
    profile.storage = ProfileStorage{
        0,
        std::numeric_limits<int64_t>::max(),
        std::numeric_limits<int64_t>::max()};
    profile.email = "none@example.com";
    profile.emailVerified = true;
    profile.scopes = {
        "assets:read",
        "assets:list",
        "assets:write",
        "profile:read",
        "tokens:read",
        "tokens:write"};
    profile.avatar = "https://www.gravatar.com/avatar/"
                     "4f14cc6c584f41d89ef1d34c8986ebfb.jpg?d=mp";
    return this->_asyncSystem.createResolvedFuture<Response<Profile>>(
        Response<Profile>{
            std::move(profile),
            200,
            std::string(),
            std::string()});
  }

  const std::string url = CesiumUtility::Uri::resolve(this->_apiUrl, "v1/me");

  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       url](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<Profile>(std::move(result)));
        }

        return pAssetAccessor
            ->get(
                asyncSystem,
                url,
                {{"Accept", "application/json"},
                 {"Authorization", *result.value}})
            .thenImmediately(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const IAssetResponse* pResponse = pRequest->response();
                  if (!pResponse) {
                    return createEmptyResponse<Profile>();
                  }

                  if (pResponse->statusCode() < 200 ||
                      pResponse->statusCode() >= 300) {
                    return createErrorResponse<Profile>(pResponse);
                  }

                  rapidjson::Document d;
                  if (!parseJsonObject(pResponse, d)) {
                    return createJsonErrorResponse<Profile>(pResponse, d);
                  }
                  if (!d.IsObject()) {
                    return createJsonTypeResponse<Profile>(pResponse, "object");
                  }

                  Profile result;

                  result.id = JsonHelpers::getInt64OrDefault(d, "id", -1);
                  result.scopes = JsonHelpers::getStrings(d, "scopes");
                  result.username =
                      JsonHelpers::getStringOrDefault(d, "username", "");
                  result.email =
                      JsonHelpers::getStringOrDefault(d, "email", "");
                  result.emailVerified =
                      JsonHelpers::getBoolOrDefault(d, "emailVerified", false);
                  result.avatar =
                      JsonHelpers::getStringOrDefault(d, "avatar", "");

                  const auto storageIt = d.FindMember("storage");
                  if (storageIt == d.MemberEnd()) {
                    result.storage.available = 0;
                    result.storage.total = 0;
                    result.storage.used = 0;
                  } else {
                    const rapidjson::Value& storage = storageIt->value;
                    result.storage.available =
                        JsonHelpers::getInt64OrDefault(storage, "available", 0);
                    result.storage.total =
                        JsonHelpers::getInt64OrDefault(storage, "total", 0);
                    result.storage.used =
                        JsonHelpers::getInt64OrDefault(storage, "used", 0);
                  }

                  return Response<Profile>{
                      std::move(result),
                      pResponse->statusCode(),
                      std::string(),
                      std::string()};
                });
      });
}

/* static */ CesiumAsync::Future<Response<ApplicationData>>
CesiumIonClient::Connection::appData(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& apiUrl) {
  return pAssetAccessor
      ->get(
          asyncSystem,
          CesiumUtility::Uri::resolve(apiUrl, "/appData"),
          {{"Accept", "application/json"}})
      .thenImmediately([](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                              pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
          return createEmptyResponse<ApplicationData>();
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
          return createErrorResponse<ApplicationData>(pResponse);
        }

        rapidjson::Document d;
        if (!parseJsonObject(pResponse, d)) {
          return createJsonErrorResponse<ApplicationData>(pResponse, d);
        }
        if (!d.IsObject()) {
          return createJsonTypeResponse<ApplicationData>(pResponse, "object");
        }

        // There's a lot more properties available on the /appData endpoint, but
        // we don't need them so we're ignoring them for now.
        ApplicationData result;
        std::string authenticationMode =
            JsonHelpers::getStringOrDefault(d, "applicationMode", "cesium-ion");
        if (authenticationMode == "single-user") {
          result.authenticationMode = AuthenticationMode::SingleUser;
        } else if (authenticationMode == "saml") {
          result.authenticationMode = AuthenticationMode::Saml;
        } else {
          result.authenticationMode = AuthenticationMode::CesiumIon;
        }
        result.dataStoreType =
            JsonHelpers::getStringOrDefault(d, "dataStoreType", "S3");
        result.attribution =
            JsonHelpers::getStringOrDefault(d, "attribution", "");

        return Response<ApplicationData>{
            std::move(result),
            pResponse->statusCode(),
            std::string(),
            std::string()};
      });
}

namespace {

Asset jsonToAsset(const rapidjson::Value& item) {
  Asset result;
  result.id = JsonHelpers::getInt64OrDefault(item, "id", -1);
  result.name = JsonHelpers::getStringOrDefault(item, "name", "");
  result.description = JsonHelpers::getStringOrDefault(item, "description", "");
  result.attribution = JsonHelpers::getStringOrDefault(item, "attribution", "");
  result.type = JsonHelpers::getStringOrDefault(item, "type", "");
  result.bytes = JsonHelpers::getInt64OrDefault(item, "bytes", -1);
  result.dateAdded = JsonHelpers::getStringOrDefault(item, "dateAdded", "");
  result.status = JsonHelpers::getStringOrDefault(item, "status", "");
  result.percentComplete =
      int8_t(JsonHelpers::getInt32OrDefault(item, "percentComplete", -1));
  return result;
}

std::optional<std::string> generateApiUrl(const std::string& ionUrl) {
  Uri parsedIonUrl(ionUrl);
  if (!parsedIonUrl) {
    return std::nullopt;
  }

  std::string url;
  url.append(parsedIonUrl.getScheme());
  url.append("//api.");
  url.append(parsedIonUrl.getHost());
  url.append("/");

  return url;
}

} // namespace

CesiumAsync::Future<std::optional<std::string>>
CesiumIonClient::Connection::getApiUrl(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& ionUrl) {
  std::string configUrl = Uri::resolve(ionUrl, "config.json");
  if (configUrl == "config.json") {
    return asyncSystem.createResolvedFuture<std::optional<std::string>>(
        std::nullopt);
  }
  return pAssetAccessor->get(asyncSystem, configUrl)
      .thenImmediately([ionUrl](std::shared_ptr<IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (pResponse && pResponse->statusCode() >= 200 &&
            pResponse->statusCode() < 300) {
          rapidjson::Document d;
          if (parseJsonObject(pResponse, d) && d.IsObject()) {
            const auto itr = d.FindMember("apiHostname");
            if (itr != d.MemberEnd() && itr->value.IsString()) {
              return std::make_optional<std::string>(
                  std::string("https://") + itr->value.GetString() + "/");
            }
          }
        }
        return generateApiUrl(ionUrl);
      })
      .catchImmediately(
          [ionUrl](std::exception&&) { return generateApiUrl(ionUrl); });
}

namespace {

QuickAddRasterOverlay
jsonToQuickAddRasterOverlay(const rapidjson::Value& json) {
  QuickAddRasterOverlay result;

  result.name = JsonHelpers::getStringOrDefault(json, "name", std::string());
  result.assetId = JsonHelpers::getInt64OrDefault(json, "assetId", -1);
  result.subscribed = JsonHelpers::getBoolOrDefault(json, "subscribed", false);

  return result;
}

QuickAddAsset jsonToQuickAddAsset(const rapidjson::Value& json) {
  QuickAddAsset result;

  result.name = JsonHelpers::getStringOrDefault(json, "name", std::string());
  result.objectName =
      JsonHelpers::getStringOrDefault(json, "objectName", std::string());
  result.description =
      JsonHelpers::getStringOrDefault(json, "description", std::string());
  result.assetId = JsonHelpers::getInt64OrDefault(json, "assetId", -1);
  result.type = JsonHelpers::getStringOrDefault(json, "type", std::string());
  result.subscribed = JsonHelpers::getBoolOrDefault(json, "subscribed", false);

  auto overlaysIt = json.FindMember("rasterOverlays");
  if (overlaysIt != json.MemberEnd() && overlaysIt->value.IsArray()) {
    const rapidjson::Value& items = overlaysIt->value;
    result.rasterOverlays.resize(items.Size());
    std::transform(
        items.Begin(),
        items.End(),
        result.rasterOverlays.begin(),
        jsonToQuickAddRasterOverlay);
  }

  return result;
}

Defaults defaultsFromJson(const rapidjson::Document& json) {
  Defaults defaults;

  auto defaultAssetsIt = json.FindMember("defaultAssets");
  if (defaultAssetsIt != json.MemberEnd() &&
      defaultAssetsIt->value.IsObject()) {
    defaults.defaultAssets.imagery =
        JsonHelpers::getInt64OrDefault(defaultAssetsIt->value, "imagery", -1);
    defaults.defaultAssets.terrain =
        JsonHelpers::getInt64OrDefault(defaultAssetsIt->value, "terrain", -1);
    defaults.defaultAssets.buildings =
        JsonHelpers::getInt64OrDefault(defaultAssetsIt->value, "buildings", -1);
  }

  auto quickAddAssetsIt = json.FindMember("quickAddAssets");
  if (quickAddAssetsIt != json.MemberEnd() &&
      quickAddAssetsIt->value.IsArray()) {
    const rapidjson::Value& items = quickAddAssetsIt->value;
    defaults.quickAddAssets.resize(items.Size());
    std::transform(
        items.Begin(),
        items.End(),
        defaults.quickAddAssets.begin(),
        jsonToQuickAddAsset);
  }

  return defaults;
}

GeocoderResult geocoderResultFromJson(const rapidjson::Document& json) {
  GeocoderResult result;

  const rapidjson::Pointer labelPointer =
      rapidjson::Pointer("/properties/label");
  const rapidjson::Pointer coordinatesPointer =
      rapidjson::Pointer("/geometry/coordinates");

  auto featuresMember = json.FindMember("features");
  if (featuresMember != json.MemberEnd() && featuresMember->value.IsArray()) {
    auto featuresIt = featuresMember->value.GetArray();
    for (auto& feature : featuresIt) {
      const rapidjson::Value* pLabel = labelPointer.Get(feature);

      std::string label;
      if (pLabel) {
        label = JsonHelpers::getStringOrDefault(*pLabel, "");
      }

      std::optional<std::vector<double>> bboxItems =
          JsonHelpers::getDoubles(feature, 4, "bbox");
      if (!bboxItems) {
        // Could be a point value.
        const rapidjson::Value* pCoordinates = coordinatesPointer.Get(feature);

        CesiumGeospatial::Cartographic point(0, 0);
        if (pCoordinates && pCoordinates->IsArray() &&
            pCoordinates->Size() == 2) {
          auto coordinatesArray = pCoordinates->GetArray();

          point = CesiumGeospatial::Cartographic::fromDegrees(
              JsonHelpers::getDoubleOrDefault(coordinatesArray[0], 0),
              JsonHelpers::getDoubleOrDefault(coordinatesArray[1], 0));
        }

        result.features.emplace_back(GeocoderFeature{label, point});
      } else {
        std::vector<double>& values = bboxItems.value();
        CesiumGeospatial::GlobeRectangle rect =
            CesiumGeospatial::GlobeRectangle::fromDegrees(
                values[0],
                values[1],
                values[2],
                values[3]);

        result.features.emplace_back(GeocoderFeature{label, rect});
      }
    }
  }

  auto attributionMemberIt = json.FindMember("attributions");
  if (attributionMemberIt != json.MemberEnd() &&
      attributionMemberIt->value.IsArray()) {
    const auto& valueJson = attributionMemberIt->value;

    result.attributions.reserve(valueJson.Size());

    for (rapidjson::SizeType i = 0; i < valueJson.Size(); ++i) {
      const rapidjson::Value& element = valueJson[i];
      std::string html = JsonHelpers::getStringOrDefault(element, "html", "");
      bool showOnScreen =
          !JsonHelpers::getBoolOrDefault(element, "collapsible", false);
      result.attributions.emplace_back(GeocoderAttribution{html, showOnScreen});
    }
  }

  return result;
}

} // namespace

Future<Response<Defaults>> Connection::defaults() const {
  const std::string url =
      CesiumUtility::Uri::resolve(this->_apiUrl, "v1/defaults");
  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       url](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<Defaults>(std::move(result)));
        }

        return pAssetAccessor
            ->get(
                asyncSystem,
                url,
                {{"Accept", "application/json"},
                 {"Authorization", *result.value}})
            .thenImmediately(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const IAssetResponse* pResponse = pRequest->response();
                  if (!pResponse) {
                    return createEmptyResponse<Defaults>();
                  }

                  if (pResponse->statusCode() < 200 ||
                      pResponse->statusCode() >= 300) {
                    return createErrorResponse<Defaults>(pResponse);
                  }

                  rapidjson::Document d;
                  if (!parseJsonObject(pResponse, d)) {
                    return createJsonErrorResponse<Defaults>(pResponse, d);
                  }
                  if (!d.IsObject()) {
                    return createJsonTypeResponse<Defaults>(
                        pResponse,
                        "object");
                  }

                  return Response<Defaults>(
                      defaultsFromJson(d),
                      pResponse->statusCode(),
                      std::string(),
                      std::string());
                });
      });
}

CesiumAsync::Future<Response<Assets>> Connection::assets() const {
  const std::string url = Uri::resolve(this->_apiUrl, "v1/assets");

  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       url](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<Assets>(std::move(result)));
        }

        return pAssetAccessor
            ->get(
                asyncSystem,
                url,
                {{"Accept", "application/json"},
                 {"Authorization", *result.value}})
            .thenImmediately(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const IAssetResponse* pResponse = pRequest->response();
                  if (!pResponse) {
                    return createEmptyResponse<Assets>();
                  }

                  if (pResponse->statusCode() < 200 ||
                      pResponse->statusCode() >= 300) {
                    return createErrorResponse<Assets>(pResponse);
                  }

                  rapidjson::Document d;
                  if (!parseJsonObject(pResponse, d)) {
                    return createJsonErrorResponse<Assets>(pResponse, d);
                  }
                  if (!d.IsObject()) {
                    return createJsonTypeResponse<Assets>(pResponse, "object");
                  }

                  Assets result;

                  result.link = JsonHelpers::getStringOrDefault(d, "link", "");

                  const auto itemsIt = d.FindMember("items");
                  if (itemsIt != d.MemberEnd() && itemsIt->value.IsArray()) {
                    const rapidjson::Value& items = itemsIt->value;
                    result.items.resize(items.Size());
                    std::transform(
                        items.Begin(),
                        items.End(),
                        result.items.begin(),
                        jsonToAsset);
                  }

                  return Response<Assets>{
                      std::move(result),
                      pResponse->statusCode(),
                      std::string(),
                      std::string()};
                });
      });
}

namespace {

Token tokenFromJson(const rapidjson::Value& json) {

  Token token;

  token.id = JsonHelpers::getStringOrDefault(json, "id", "");
  token.name = JsonHelpers::getStringOrDefault(json, "name", "");
  token.token = JsonHelpers::getStringOrDefault(json, "token", "");
  token.dateAdded = JsonHelpers::getStringOrDefault(json, "dateAdded", "");
  token.dateModified =
      JsonHelpers::getStringOrDefault(json, "dateModified", "");
  token.dateLastUsed =
      JsonHelpers::getStringOrDefault(json, "dateLastUsed", "");
  token.isDefault = JsonHelpers::getBoolOrDefault(json, "isDefault", false);
  token.scopes = JsonHelpers::getStrings(json, "scopes");

  auto assetIdsIt = json.FindMember("assetIds");
  if (assetIdsIt != json.MemberEnd() && !assetIdsIt->value.IsNull()) {
    token.assetIds = JsonHelpers::getInt64s(json, "assetIds");
  } else {
    token.assetIds = std::nullopt;
  }

  auto allowedUrlsIt = json.FindMember("allowedUrls");
  if (allowedUrlsIt != json.MemberEnd() && !allowedUrlsIt->value.IsNull()) {
    token.allowedUrls = JsonHelpers::getStrings(json, "allowedUrls");
  } else {
    token.allowedUrls = std::nullopt;
  }

  return token;
}

TokenList tokenListFromJson(const rapidjson::Value& json) {
  TokenList result;

  const auto itemsIt = json.FindMember("items");
  if (itemsIt != json.MemberEnd() && itemsIt->value.IsArray()) {
    const rapidjson::Value& items = itemsIt->value;

    for (auto it = items.Begin(); it != items.End(); ++it) {
      std::optional<Token> token = tokenFromJson(*it);
      if (!token) {
        continue;
      }

      result.items.emplace_back(std::move(token.value()));
    }
  }

  return result;
}

} // namespace

Future<Response<TokenList>>
Connection::tokens(const ListTokensOptions& options) const {
  if (this->_appData.authenticationMode == AuthenticationMode::SingleUser) {
    TokenList emptyList = TokenList{};
    return this->_asyncSystem.createResolvedFuture<Response<TokenList>>(
        Response{std::move(emptyList), 200, "", ""});
  }

  std::string url = Uri::resolve(this->_apiUrl, "v2/tokens");

  if (options.limit) {
    url = Uri::addQuery(url, "limit", std::to_string(*options.limit));
  }
  if (options.page) {
    url = Uri::addQuery(url, "page", std::to_string(*options.page));
  }
  if (options.search) {
    url = Uri::addQuery(url, "search", *options.search);
  }
  if (options.sortBy) {
    url = Uri::addQuery(url, "sortBy", *options.sortBy);
  }
  if (options.sortOrder) {
    url = Uri::addQuery(
        url,
        "sortOrder",
        *options.sortOrder == SortOrder::Ascending ? "ASC" : "DESC");
  }

  return this->tokens(url);
}

CesiumAsync::Future<Response<Asset>> Connection::asset(int64_t assetID) const {
  std::string assetsUrl =
      CesiumUtility::Uri::resolve(this->_apiUrl, "v1/assets/");

  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       assetsUrl,
       assetID](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<Asset>(std::move(result)));
        }

        return pAssetAccessor
            ->get(
                asyncSystem,
                CesiumUtility::Uri::resolve(assetsUrl, std::to_string(assetID)),
                {{"Accept", "application/json"},
                 {"Authorization", *result.value}})
            .thenImmediately(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const IAssetResponse* pResponse = pRequest->response();
                  if (!pResponse) {
                    return createEmptyResponse<Asset>();
                  }

                  if (pResponse->statusCode() < 200 ||
                      pResponse->statusCode() >= 300) {
                    return createErrorResponse<Asset>(pResponse);
                  }

                  rapidjson::Document d;
                  if (!parseJsonObject(pResponse, d)) {
                    return createJsonErrorResponse<Asset>(pResponse, d);
                  }
                  if (!d.IsObject()) {
                    return createJsonTypeResponse<Asset>(pResponse, "object");
                  }

                  return Response<Asset>{
                      jsonToAsset(d),
                      pResponse->statusCode(),
                      std::string(),
                      std::string()};
                });
      });
}

CesiumAsync::Future<Response<Token>>
Connection::token(const std::string& tokenID) const {
  std::string tokensUrl =
      CesiumUtility::Uri::resolve(this->_apiUrl, "v2/tokens/");

  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       tokensUrl,
       tokenID](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<Token>(std::move(result)));
        }

        return pAssetAccessor
            ->get(
                asyncSystem,
                CesiumUtility::Uri::resolve(tokensUrl, tokenID),
                {{"Accept", "application/json"},
                 {"Authorization", *result.value}})
            .thenImmediately(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const IAssetResponse* pResponse = pRequest->response();
                  if (!pResponse) {
                    return createEmptyResponse<Token>();
                  }

                  if (pResponse->statusCode() < 200 ||
                      pResponse->statusCode() >= 300) {
                    return createErrorResponse<Token>(pResponse);
                  }

                  rapidjson::Document d;
                  if (!parseJsonObject(pResponse, d)) {
                    return createJsonErrorResponse<Token>(pResponse, d);
                  }
                  if (!d.IsObject()) {
                    return createJsonTypeResponse<Token>(pResponse, "object");
                  }

                  return Response<Token>{
                      tokenFromJson(d),
                      pResponse->statusCode(),
                      std::string(),
                      std::string()};
                });
      });
}

CesiumAsync::Future<Response<TokenList>>
Connection::nextPage(const Response<TokenList>& currentPage) const {
  if (!currentPage.nextPageUrl) {
    return this->_asyncSystem.createResolvedFuture(
        createNoMorePagesResponse<TokenList>());
  }

  return this->tokens(*currentPage.nextPageUrl);
}

CesiumAsync::Future<Response<TokenList>>
Connection::previousPage(const Response<TokenList>& currentPage) const {
  if (!currentPage.previousPageUrl) {
    return this->_asyncSystem.createResolvedFuture(
        createNoMorePagesResponse<TokenList>());
  }

  return this->tokens(*currentPage.previousPageUrl);
}

CesiumAsync::Future<Response<Token>> Connection::createToken(
    const std::string& name,
    const std::vector<std::string>& scopes,
    const std::optional<std::vector<int64_t>>& assetIds,
    const std::optional<std::vector<std::string>>& allowedUrls) const {
  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       apiUrl = this->_apiUrl,
       name,
       scopes,
       assetIds,
       allowedUrls](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<Token>(std::move(result)));
        }

        rapidjson::StringBuffer tokenBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(tokenBuffer);

        writer.StartObject();

        writer.Key("name");
        writer.String(name.c_str(), rapidjson::SizeType(name.size()));

        writer.Key("scopes");
        writer.StartArray();
        for (auto& scope : scopes) {
          writer.String(scope.c_str(), rapidjson::SizeType(scope.size()));
        }
        writer.EndArray();

        writer.Key("assetIds");
        if (assetIds) {
          writer.StartArray();
          for (auto asset : assetIds.value()) {
            writer.Int64(asset);
          }
          writer.EndArray();
        } else {
          writer.Null();
        }

        writer.Key("allowedUrls");
        if (allowedUrls) {
          writer.StartArray();
          for (auto& allowedUrl : allowedUrls.value()) {
            writer.String(
                allowedUrl.c_str(),
                rapidjson::SizeType(allowedUrl.size()));
          }
          writer.EndArray();
        } else {
          writer.Null();
        }

        writer.EndObject();

        const std::span<const std::byte> tokenBytes(
            reinterpret_cast<const std::byte*>(tokenBuffer.GetString()),
            tokenBuffer.GetSize());

        const std::string url = Uri::resolve(apiUrl, "v2/tokens");

        return pAssetAccessor
            ->request(
                asyncSystem,
                "POST",
                url,
                {{"Content-Type", "application/json"},
                 {"Accept", "application/json"},
                 {"Authorization", *result.value}},
                tokenBytes)
            .thenImmediately(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const IAssetResponse* pResponse = pRequest->response();
                  if (!pResponse) {
                    return createEmptyResponse<Token>();
                  }

                  if (pResponse->statusCode() < 200 ||
                      pResponse->statusCode() >= 300) {
                    return createErrorResponse<Token>(pResponse);
                  }

                  rapidjson::Document d;
                  if (!parseJsonObject(pResponse, d)) {
                    return createJsonErrorResponse<Token>(pResponse, d);
                  }
                  if (!d.IsObject()) {
                    return createJsonTypeResponse<Token>(pResponse, "object");
                  }

                  Token token = tokenFromJson(d);
                  return Response<Token>{
                      std::move(token),
                      pResponse->statusCode(),
                      std::string(),
                      std::string()};
                });
      });
}

Future<Response<NoValue>> Connection::modifyToken(
    const std::string& tokenID,
    const std::string& newName,
    const std::optional<std::vector<int64_t>>& newAssetIDs,
    const std::vector<std::string>& newScopes,
    const std::optional<std::vector<std::string>>& newAllowedUrls) const {
  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       apiUrl = this->_apiUrl,
       tokenID,
       newName,
       newAssetIDs,
       newScopes,
       newAllowedUrls](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<NoValue>(std::move(result)));
        }

        std::string url = Uri::resolve(apiUrl, "v2/tokens/");
        url = Uri::resolve(url, tokenID);

        rapidjson::StringBuffer tokenBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(tokenBuffer);

        writer.StartObject();

        writer.Key("name");
        writer.String(newName.c_str(), rapidjson::SizeType(newName.size()));

        writer.Key("assetIds");
        if (newAssetIDs) {
          writer.StartArray();
          for (auto asset : newAssetIDs.value()) {
            writer.Int64(asset);
          }
          writer.EndArray();
        } else {
          writer.Null();
        }

        writer.Key("scopes");
        writer.StartArray();
        for (auto& scope : newScopes) {
          writer.String(scope.c_str(), rapidjson::SizeType(scope.size()));
        }
        writer.EndArray();

        writer.Key("newAllowedUrls");
        if (newAllowedUrls) {
          writer.StartArray();
          for (auto& allowedUrl : newAllowedUrls.value()) {
            writer.String(
                allowedUrl.c_str(),
                rapidjson::SizeType(allowedUrl.size()));
          }
          writer.EndArray();
        } else {
          writer.Null();
        }

        writer.EndObject();

        const std::span<const std::byte> tokenBytes(
            reinterpret_cast<const std::byte*>(tokenBuffer.GetString()),
            tokenBuffer.GetSize());

        return pAssetAccessor
            ->request(
                asyncSystem,
                "PATCH",
                url,
                {{"Content-Type", "application/json"},
                 {"Accept", "application/json"},
                 {"Authorization", *result.value}},
                tokenBytes)
            .thenImmediately([](std::shared_ptr<IAssetRequest>&& pRequest) {
              const IAssetResponse* pResponse = pRequest->response();
              if (!pResponse) {
                return createEmptyResponse<NoValue>();
              }

              if (pResponse->statusCode() < 200 ||
                  pResponse->statusCode() >= 300) {
                return createErrorResponse<NoValue>(pResponse);
              }

              return Response<NoValue>{
                  NoValue(),
                  pResponse->statusCode(),
                  std::string(),
                  std::string()};
            });
      });
}

/*static*/ std::optional<std::string>
Connection::getIdFromToken(const std::string& token) {
  Result<rapidjson::Document> parseResult =
      CesiumClientCommon::JwtTokenUtility::parseTokenPayload(token);
  if (!parseResult.value) {
    return std::nullopt;
  }

  auto jtiIt = parseResult.value->FindMember("jti");
  if (jtiIt == parseResult.value->MemberEnd()) {
    return std::nullopt;
  }

  if (!jtiIt->value.IsString()) {
    return std::nullopt;
  }

  return jtiIt->value.GetString();
}

CesiumAsync::Future<Response<TokenList>>
Connection::tokens(const std::string& url) const {
  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       url](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<TokenList>(std::move(result)));
        }

        return pAssetAccessor
            ->get(
                asyncSystem,
                url,
                {{"Accept", "application/json"},
                 {"Authorization", *result.value}})
            .thenImmediately(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const IAssetResponse* pResponse = pRequest->response();
                  if (!pResponse) {
                    return createEmptyResponse<TokenList>();
                  }

                  if (pResponse->statusCode() < 200 ||
                      pResponse->statusCode() >= 300) {
                    return createErrorResponse<TokenList>(pResponse);
                  }

                  rapidjson::Document d;
                  if (!parseJsonObject(pResponse, d)) {
                    return createJsonErrorResponse<TokenList>(pResponse, d);
                  }
                  if (!d.IsObject()) {
                    return createJsonTypeResponse<TokenList>(
                        pResponse,
                        "object");
                  }

                  return Response<TokenList>(pRequest, tokenListFromJson(d));
                });
      });
}

CesiumAsync::Future<Response<GeocoderResult>> Connection::geocode(
    GeocoderProviderType provider,
    GeocoderRequestType type,
    const std::string& query) const {
  const std::string endpointUrl = type == GeocoderRequestType::Autocomplete
                                      ? "v1/geocode/autocomplete"
                                      : "v1/geocode/search";
  std::string requestUrl =
      CesiumUtility::Uri::resolve(this->_apiUrl, endpointUrl);
  requestUrl = CesiumUtility::Uri::addQuery(requestUrl, "text", query);

  // Add provider type to url
  switch (provider) {
  case GeocoderProviderType::Bing:
    requestUrl = CesiumUtility::Uri::addQuery(requestUrl, "geocoder", "bing");
    break;
  case GeocoderProviderType::Google:
    requestUrl = CesiumUtility::Uri::addQuery(requestUrl, "geocoder", "google");
    break;
  case GeocoderProviderType::Default:
    break;
  }

  return this->ensureValidToken().thenImmediately(
      [pAssetAccessor = this->_pAssetAccessor,
       asyncSystem = this->_asyncSystem,
       requestUrl](Result<std::string>&& result) {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              createNoValidTokenResponse<GeocoderResult>(std::move(result)));
        }

        return pAssetAccessor
            ->get(
                asyncSystem,
                requestUrl,
                {{"Accept", "application/json"},
                 {"Authorization", *result.value}})
            .thenImmediately(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const IAssetResponse* pResponse = pRequest->response();
                  if (!pResponse) {
                    return createEmptyResponse<GeocoderResult>();
                  }

                  if (pResponse->statusCode() < 200 ||
                      pResponse->statusCode() >= 300) {
                    return createErrorResponse<GeocoderResult>(pResponse);
                  }

                  rapidjson::Document d;
                  if (!parseJsonObject(pResponse, d)) {
                    return createJsonErrorResponse<GeocoderResult>(
                        pResponse,
                        d);
                  }
                  if (!d.IsObject()) {
                    return createJsonTypeResponse<GeocoderResult>(
                        pResponse,
                        "object");
                  }

                  return Response<GeocoderResult>(
                      geocoderResultFromJson(d),
                      pResponse->statusCode(),
                      std::string(),
                      std::string());
                });
      });
}

CesiumAsync::Future<Result<std::string>> Connection::ensureValidToken() const {
  std::lock_guard lock(this->_pTokenDetails->mutex);

  if (this->_pTokenDetails->accessToken.isValid()) {
    return this->_asyncSystem.createResolvedFuture(Result<std::string>(
        "Bearer " + this->_pTokenDetails->accessToken.getToken()));
  }

  if (this->_pTokenDetails->refreshToken.empty()) {
    return this->_asyncSystem.createResolvedFuture(Result<std::string>(
        ErrorList::error("Cesium ion access token has expired and no refresh "
                         "token is present.")));
  }

  if (!this->_pTokenDetails->refreshInProgress) {
    const std::string tokenUrl = Uri::resolve(this->_apiUrl, "oauth/token");

    this->_pTokenDetails->refreshInProgress =
        CesiumClientCommon::OAuth2PKCE::refresh(
            this->_asyncSystem,
            this->_pAssetAccessor,
            CesiumClientCommon::OAuth2ClientOptions{
                std::to_string(this->_clientId),
                this->_redirectPath,
                std::nullopt,
                true},
            tokenUrl,
            this->_pTokenDetails->refreshToken)
            .thenInMainThread([pDetails = this->_pTokenDetails](
                                  Result<
                                      CesiumClientCommon::OAuth2TokenResponse>&&
                                      result) {
              std::lock_guard lock(pDetails->mutex);

              pDetails->accessToken = LoginToken("", -1);
              pDetails->refreshToken.clear();
              pDetails->refreshInProgress.reset();

              ErrorList combinedList = ErrorList::error(
                  "Token refresh failed. Please sign into Cesium ion again.");
              if (!result.value) {
                combinedList.merge(result.errors);
                return Result<std::string>(combinedList);
              }

              Result<LoginToken> tokenResult =
                  LoginToken::parse(result.value->accessToken);
              if (!tokenResult.value) {
                combinedList.merge(tokenResult.errors);
                return Result<std::string>(combinedList);
              }

              pDetails->accessToken = std::move(*tokenResult.value);
              pDetails->refreshToken = result.value->refreshToken.value_or("");

              return Result<std::string>(
                  "Bearer " + pDetails->accessToken.getToken());
            })
            .share();
  }

  // The thenImmediately only exists to turn the SharedFuture into a regular
  // Future.
  return this->_pTokenDetails->refreshInProgress->thenImmediately(
      [](auto&& value) { return std::move(value); });
}

Connection::TokenDetails::TokenDetails(
    const CesiumIonClient::LoginToken& accessToken,
    const std::string& refreshToken)
    : accessToken(accessToken),
      refreshToken(refreshToken),
      refreshInProgress(),
      mutex() {}
