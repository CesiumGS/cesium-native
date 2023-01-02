#include "CesiumIonClient/Connection.h"

#include "parseLinkHeader.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <duthomhas/csprng.hpp>
#include <httplib.h>
#include <modp_b64.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <thread>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#include <picosha2.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace CesiumAsync;
using namespace CesiumIonClient;
using namespace CesiumUtility;

namespace {
std::string encodeBase64(const std::vector<uint8_t>& bytes) {
  const size_t count = modp_b64_encode_len(bytes.size());
  std::string result(count, 0);
  const size_t actualLength = modp_b64_encode(
      result.data(),
      reinterpret_cast<const char*>(bytes.data()),
      bytes.size());
  result.resize(actualLength);

  // Convert to a URL-friendly form of Base64 according to the algorithm
  // in [RFC7636 Appendix A](https://tools.ietf.org/html/rfc7636#appendix-A)
  const size_t firstPaddingIndex = result.find('=');
  if (firstPaddingIndex != std::string::npos) {
    result.erase(result.begin() + int64_t(firstPaddingIndex), result.end());
  }
  std::replace(result.begin(), result.end(), '+', '-');
  std::replace(result.begin(), result.end(), '/', '_');

  return result;
}

std::string createSuccessHtml(const std::string& applicationName) {
  return std::string("<html>") +
         "<h2 style=\"text-align: center;\">Successfully "
         "authorized!</h2><br/>" +
         "<div style=\"text-align: center;\">Please close this window and "
         "return to " +
         applicationName + ".</div>" + "<html>";
}

std::string createGenericErrorHtml(
    const std::string& applicationName,
    const std::string& errorMessage,
    const std::string& errorDescription) {
  return std::string("<html>") + "<h2 style=\"text-align: center;\">" +
         errorMessage + "</h2><br/>" + "<div style=\"text-align: center;\">" +
         errorDescription + ".</div><br/>" +
         "<div style=\"text-align: center;\">Please close this window and "
         "return to " +
         applicationName + " to try again.</div>" + "<html>";
}

std::string createAuthorizationErrorHtml(
    const std::string& applicationName,
    const std::exception& exception) {
  return std::string("<html>") +
         "<h2 style=\"text-align: center;\">Not authorized!</h2><br/>" +
         "<div style=\"text-align: center;\">The authorization failed with the "
         "following error message: " +
         exception.what() + ".</div><br/>" +
         "<div style=\"text-align: center;\">Please close this window and "
         "return to " +
         applicationName + ".</div><br/>" +
         "<div style=\"text-align: center;\">If the problem persists, contact "
         "our support at <a "
         "href=\"mailto:support@cesium.com\">support@cesium.com</a>.</div>" +
         "<html>";
}

} // namespace

/*static*/ CesiumAsync::Future<Connection> Connection::authorize(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& friendlyApplicationName,
    int64_t clientID,
    const std::string& redirectPath,
    const std::vector<std::string>& scopes,
    std::function<void(const std::string&)>&& openUrlCallback,
    const std::string& ionApiUrl,
    const std::string& ionAuthorizeUrl) {

  auto promise = asyncSystem.createPromise<Connection>();

  std::shared_ptr<httplib::Server> pServer =
      std::make_shared<httplib::Server>();
  const int port = pServer->bind_to_any_port("127.0.0.1");

  std::string redirectUrl =
      Uri::resolve("http://127.0.0.1:" + std::to_string(port), redirectPath);

  duthomhas::csprng rng;
  std::vector<uint8_t> stateBytes(32, 0);
  rng(stateBytes);

  std::string state = encodeBase64(stateBytes);

  std::vector<uint8_t> codeVerifierBytes(32, 0);
  rng(codeVerifierBytes);

  std::string codeVerifier = encodeBase64(codeVerifierBytes);

  std::vector<uint8_t> hashedChallengeBytes(picosha2::k_digest_size);
  picosha2::hash256(codeVerifier, hashedChallengeBytes);
  std::string hashedChallenge = encodeBase64(hashedChallengeBytes);

  std::string authorizeUrl = ionAuthorizeUrl;
  authorizeUrl = Uri::addQuery(authorizeUrl, "response_type", "code");
  authorizeUrl =
      Uri::addQuery(authorizeUrl, "client_id", std::to_string(clientID));
  authorizeUrl =
      Uri::addQuery(authorizeUrl, "scope", joinToString(scopes, " "));
  authorizeUrl = Uri::addQuery(authorizeUrl, "redirect_uri", redirectUrl);
  authorizeUrl = Uri::addQuery(authorizeUrl, "state", state);
  authorizeUrl = Uri::addQuery(authorizeUrl, "code_challenge_method", "S256");
  authorizeUrl = Uri::addQuery(authorizeUrl, "code_challenge", hashedChallenge);

  pServer->Get(
      redirectPath,
      [promise,
       pServer,
       asyncSystem,
       pAssetAccessor,
       friendlyApplicationName,
       clientID,
       ionApiUrl,
       redirectUrl,
       expectedState = state,
       codeVerifier](
          const httplib::Request& request,
          httplib::Response& response) {
        pServer->stop();

        std::string error = request.get_param_value("error");
        std::string errorDescription =
            request.get_param_value("error_description");
        if (!error.empty()) {
          std::string errorMessage = "Error";
          std::string errorDescriptionMessage = "An unknown error occurred";
          if (error == "access_denied") {
            errorMessage = "Access denied";
          }
          if (!errorDescription.empty()) {
            errorDescriptionMessage = errorDescription;
          }
          response.set_content(
              createGenericErrorHtml(
                  friendlyApplicationName,
                  errorMessage,
                  errorDescriptionMessage),
              "text/html");
          promise.reject(std::runtime_error("Received an error message"));
          return;
        }

        std::string code = request.get_param_value("code");
        std::string state = request.get_param_value("state");
        if (state != expectedState) {
          response.set_content(
              createGenericErrorHtml(
                  friendlyApplicationName,
                  "Invalid state",
                  "The redirection received an invalid state"),
              "text/html");
          promise.reject(std::runtime_error("Received an invalid state."));
          return;
        }

        try {
          Connection connection = Connection::completeTokenExchange(
                                      asyncSystem,
                                      pAssetAccessor,
                                      clientID,
                                      ionApiUrl,
                                      code,
                                      redirectUrl,
                                      codeVerifier)
                                      .wait();

          response.set_content(
              createSuccessHtml(friendlyApplicationName),
              "text/html");
          promise.resolve(std::move(connection));
        } catch (const std::exception& exception) {
          response.set_content(
              createAuthorizationErrorHtml(friendlyApplicationName, exception),
              "text/html");
          promise.reject(std::current_exception());
        } catch (...) {
          response.set_content(
              createAuthorizationErrorHtml(
                  friendlyApplicationName,
                  std::runtime_error("Unknown error")),
              "text/html");
          promise.reject(std::current_exception());
        }
      });

  // TODO: Make this process cancelable, and shut down the server when it's
  // canceled.
  std::thread([pServer, authorizeUrl]() {
    pServer->listen_after_bind();
  }).detach();

  openUrlCallback(authorizeUrl);

  return promise.getFuture();
}

Connection::Connection(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& accessToken,
    const std::string& apiUrl)
    : _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _accessToken(accessToken),
      _apiUrl(apiUrl) {}

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

bool parseJsonObject(const IAssetResponse* pResponse, rapidjson::Document& d) {
  d.Parse(
      reinterpret_cast<const char*>(pResponse->data().data()),
      pResponse->data().size());
  return !d.HasParseError();
}

} // namespace

CesiumAsync::Future<Response<Profile>> Connection::me() const {
  return this->_pAssetAccessor
      ->get(
          this->_asyncSystem,
          CesiumUtility::Uri::resolve(this->_apiUrl, "v1/me"),
          {{"Accept", "application/json"},
           {"Authorization", "Bearer " + this->_accessToken}})
      .thenInMainThread(
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
            result.email = JsonHelpers::getStringOrDefault(d, "email", "");
            result.emailVerified =
                JsonHelpers::getBoolOrDefault(d, "emailVerified", false);
            result.avatar = JsonHelpers::getStringOrDefault(d, "avatar", "");

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

} // namespace

CesiumAsync::Future<Response<Assets>> Connection::assets() const {
  return this->_pAssetAccessor
      ->get(
          this->_asyncSystem,
          CesiumUtility::Uri::resolve(this->_apiUrl, "v1/assets"),
          {{"Accept", "application/json"},
           {"Authorization", "Bearer " + this->_accessToken}})
      .thenInMainThread(
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
  if (itemsIt != json.MemberEnd() || !itemsIt->value.IsArray()) {
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

  return this->_pAssetAccessor
      ->get(
          this->_asyncSystem,
          CesiumUtility::Uri::resolve(assetsUrl, std::to_string(assetID)),
          {{"Accept", "application/json"},
           {"Authorization", "Bearer " + this->_accessToken}})
      .thenInMainThread(
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
}

CesiumAsync::Future<Response<Token>>
Connection::token(const std::string& tokenID) const {
  std::string tokensUrl =
      CesiumUtility::Uri::resolve(this->_apiUrl, "v2/tokens/");

  return this->_pAssetAccessor
      ->get(
          this->_asyncSystem,
          CesiumUtility::Uri::resolve(tokensUrl, tokenID),
          {{"Accept", "application/json"},
           {"Authorization", "Bearer " + this->_accessToken}})
      .thenInMainThread(
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
      writer.String(allowedUrl.c_str(), rapidjson::SizeType(allowedUrl.size()));
    }
    writer.EndArray();
  } else {
    writer.Null();
  }

  writer.EndObject();

  const gsl::span<const std::byte> tokenBytes(
      reinterpret_cast<const std::byte*>(tokenBuffer.GetString()),
      tokenBuffer.GetSize());
  return this->_pAssetAccessor
      ->request(
          this->_asyncSystem,
          "POST",
          Uri::resolve(this->_apiUrl, "v2/tokens"),
          {{"Content-Type", "application/json"},
           {"Accept", "application/json"},
           {"Authorization", "Bearer " + this->_accessToken}},
          tokenBytes)
      .thenInMainThread(
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
}

Future<Response<NoValue>> Connection::modifyToken(
    const std::string& tokenID,
    const std::string& newName,
    const std::optional<std::vector<int64_t>>& newAssetIDs,
    const std::vector<std::string>& newScopes,
    const std::optional<std::vector<std::string>>& newAllowedUrls) const {
  std::string url = Uri::resolve(this->_apiUrl, "v2/tokens/");
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
      writer.String(allowedUrl.c_str(), rapidjson::SizeType(allowedUrl.size()));
    }
    writer.EndArray();
  } else {
    writer.Null();
  }

  writer.EndObject();

  const gsl::span<const std::byte> tokenBytes(
      reinterpret_cast<const std::byte*>(tokenBuffer.GetString()),
      tokenBuffer.GetSize());

  return this->_pAssetAccessor
      ->request(
          this->_asyncSystem,
          "PATCH",
          url,
          {{"Content-Type", "application/json"},
           {"Accept", "application/json"},
           {"Authorization", "Bearer " + this->_accessToken}},
          tokenBytes)
      .thenInMainThread([](std::shared_ptr<IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
          return createEmptyResponse<NoValue>();
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
          return createErrorResponse<NoValue>(pResponse);
        }

        return Response<NoValue>{
            NoValue(),
            pResponse->statusCode(),
            std::string(),
            std::string()};
      });
}

/*static*/ std::optional<std::string>
Connection::getIdFromToken(const std::string& token) {
  size_t startPos = token.find(".");
  if (startPos == std::string::npos || startPos == token.size() - 1) {
    return std::nullopt;
  }

  size_t endPos = token.find(".", startPos + 1);
  if (endPos == std::string::npos) {
    return std::nullopt;
  }

  size_t length = endPos - startPos - 1;
  if (length == 0) {
    return std::nullopt;
  }

  std::string encoded(
      token.begin() + std::string::difference_type(startPos + 1),
      token.begin() + std::string::difference_type(endPos));

  // Add base64 padding, as required by modp_b64_decode.
  size_t remainder = encoded.size() % 4;
  if (remainder != 0) {
    encoded.resize(encoded.size() + 4 - remainder, '=');
  }

  std::string decoded(modp_b64_decode_len(length), '\0');
  size_t decodedLength =
      modp_b64_decode(decoded.data(), encoded.data(), encoded.size());
  if (decodedLength == 0 || decodedLength == std::string::npos) {
    return std::nullopt;
  }

  decoded.resize(decodedLength);

  rapidjson::Document document;
  document.Parse(decoded.data(), decoded.size());
  if (document.HasParseError()) {
    return std::nullopt;
  }

  auto jtiIt = document.FindMember("jti");
  if (jtiIt == document.MemberEnd()) {
    return std::nullopt;
  }

  if (!jtiIt->value.IsString()) {
    return std::nullopt;
  }

  return jtiIt->value.GetString();
}

/*static*/ CesiumAsync::Future<Connection> Connection::completeTokenExchange(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    int64_t clientID,
    const std::string& ionApiUrl,
    const std::string& code,
    const std::string& redirectUrl,
    const std::string& codeVerifier) {
  rapidjson::StringBuffer postBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(postBuffer);

  writer.StartObject();
  writer.Key("grant_type");
  writer.String("authorization_code");
  writer.Key("client_id");
  writer.String(std::to_string(clientID).c_str());
  writer.Key("code");
  writer.String(code.c_str(), rapidjson::SizeType(code.size()));
  writer.Key("redirect_uri");
  writer.String(redirectUrl.c_str(), rapidjson::SizeType(redirectUrl.size()));
  writer.Key("code_verifier");
  writer.String(codeVerifier.c_str(), rapidjson::SizeType(codeVerifier.size()));
  writer.EndObject();

  const gsl::span<const std::byte> payload(
      reinterpret_cast<const std::byte*>(postBuffer.GetString()),
      postBuffer.GetSize());

  return pAssetAccessor
      ->request(
          asyncSystem,
          "POST",
          Uri::resolve(ionApiUrl, "oauth/token"),
          {{"Content-Type", "application/json"},
           {"Accept", "application/json"}},
          payload)
      .thenInWorkerThread([asyncSystem, pAssetAccessor, ionApiUrl](
                              std::shared_ptr<IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
          throw std::runtime_error("The server did not return a response.");
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
          throw std::runtime_error(
              "The server returned an error code: " +
              std::to_string(pResponse->statusCode()));
        }

        rapidjson::Document d;
        d.Parse(
            reinterpret_cast<const char*>(pResponse->data().data()),
            pResponse->data().size());
        if (d.HasParseError()) {
          throw std::runtime_error(
              std::string("Failed to parse JSON response: ") +
              rapidjson::GetParseError_En(d.GetParseError()));
        }

        std::string accessToken =
            JsonHelpers::getStringOrDefault(d, "access_token", "");
        if (accessToken.empty()) {
          throw std::runtime_error(
              "Server response does not include a valid token.");
        }

        return Connection(asyncSystem, pAssetAccessor, accessToken, ionApiUrl);
      });
}

CesiumAsync::Future<Response<TokenList>>
Connection::tokens(const std::string& url) const {
  return this->_pAssetAccessor
      ->get(
          this->_asyncSystem,
          url,
          {{"Accept", "application/json"},
           {"Authorization", "Bearer " + this->_accessToken}})
      .thenInMainThread(
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
              return createJsonTypeResponse<TokenList>(pResponse, "object");
            }

            return Response<TokenList>(pRequest, tokenListFromJson(d));
          });
}
