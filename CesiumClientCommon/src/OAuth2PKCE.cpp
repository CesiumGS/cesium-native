#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumClientCommon/ErrorResponse.h>
#include <CesiumClientCommon/OAuth2PKCE.h>
#include <CesiumClientCommon/fillWithRandomBytes.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <fmt/format.h>
#include <httplib.h>
#include <modp_b64.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ >= 10)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#include <picosha2.h>

#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ >= 10)
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace CesiumUtility;

namespace CesiumClientCommon {

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
  return fmt::format(
      R"STR(
      <html>
        <h2 style="text-align: center;">Successfully authorized!</h2>
        <br/>
        <div style="text-align: center;">
          Please close this window and return to {}.
        </div>
      <html>
      )STR",
      applicationName);
}

std::string createGenericErrorHtml(
    const std::string& applicationName,
    const std::string& errorMessage,
    const std::string& errorDescription) {
  return fmt::format(
      R"STR(
      <html>
        <h2 style="text-align: center;">{}</h2>
        <br/>
        <div style="text-align: center;">
          {}
        </div>
        <br/>
        <div style="text-align: center;">
          Please close this window and return to {}.
        </div>
      <html>
      )STR",
      errorMessage,
      errorDescription,
      applicationName);
}

std::string createGenericErrorHtml(
    const std::string& applicationName,
    const std::string& title,
    const ErrorList& errors) {
  std::string errorsInnerHtml;
  if (errors.hasErrors()) {
    for (const std::string& error : errors.errors) {
      errorsInnerHtml.append(fmt::format("\t<li>{}</li>\n", error));
    }
  }
  const std::string errorsHtml = fmt::format("<ul>\n{}</ul>", errorsInnerHtml);
  return fmt::format(
      R"STR(
      <html>
        <h2 style="text-align: center;">{}</h2>
        <br/>
        <div style="text-align: center;">
          {}
        </div>
        <br/>
        <div style="text-align: center;">
          Please close this window and return to {}.
        </div>
      <html>
      )STR",
      title,
      errorsHtml,
      applicationName);
}

std::string createAuthorizationErrorHtml(
    const std::string& applicationName,
    const std::exception& exception) {
  return fmt::format(
      R"STR(
      <html>
        <h2 style="text-align: center;">Not authorized!</h2>
        <br/>
        <div style="text-align: center;">
          The authorization failed with the following error message: {}.
        </div>
        <br/>
        <div style="text-align: center;">
          Please close this window and return to {}.
        </div>
        <br/>
        <div style="text-align: center;">
          If the problem persists, contact our support at <a href="mailto:support@cesium.com">support@cesium.com</a>.
        </div>
      </html>)STR",
      exception.what(),
      applicationName);
}
} // namespace

CesiumAsync::Future<Result<OAuth2TokenResponse>> OAuth2PKCE::authorize(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& friendlyApplicationName,
    const OAuth2ClientOptions& clientOptions,
    const std::vector<std::string>& scopes,
    std::function<void(const std::string&)>&& openUrlCallback,
    const std::string& tokenEndpointUrl,
    const std::string& authorizeBaseUrl) {
  auto promise = asyncSystem.createPromise<Result<OAuth2TokenResponse>>();

  std::shared_ptr<httplib::Server> pServer =
      std::make_shared<httplib::Server>();
  int port;
  if (clientOptions.redirectPort) {
    port = *clientOptions.redirectPort;
    if (!pServer->bind_to_port("127.0.0.1", port)) {
      promise.resolve(Result<OAuth2TokenResponse>(ErrorList::error(fmt::format(
          "Internal HTTP server failed to bind to port {}.",
          port))));
      return promise.getFuture();
    }
  } else {
    port = pServer->bind_to_any_port("127.0.0.1");
  }

  std::string redirectUrl = Uri::resolve(
      "http://127.0.0.1:" + std::to_string(port),
      clientOptions.redirectPath);

  std::vector<uint8_t> stateBytes(32, 0);
  fillWithRandomBytes(stateBytes);

  std::string state = encodeBase64(stateBytes);

  std::vector<uint8_t> codeVerifierBytes(32, 0);
  fillWithRandomBytes(codeVerifierBytes);

  std::string codeVerifier = encodeBase64(codeVerifierBytes);

  std::vector<uint8_t> hashedChallengeBytes(picosha2::k_digest_size);
  picosha2::hash256(codeVerifier, hashedChallengeBytes);
  std::string hashedChallenge = encodeBase64(hashedChallengeBytes);

  Uri authorizeUri(authorizeBaseUrl);
  UriQuery authorizeUriQuery(authorizeUri.getQuery());
  authorizeUriQuery.setValue("response_type", "code");
  authorizeUriQuery.setValue("client_id", clientOptions.clientID);
  authorizeUriQuery.setValue("scope", joinToString(scopes, " "));
  authorizeUriQuery.setValue("redirect_uri", redirectUrl);
  authorizeUriQuery.setValue("state", state);
  authorizeUriQuery.setValue("code_challenge_method", "S256");
  authorizeUriQuery.setValue("code_challenge", hashedChallenge);
  authorizeUri.setQuery(authorizeUriQuery.toQueryString());

  const std::string authorizeUrl = std::string(authorizeUri.toString());

  pServer->Get(
      clientOptions.redirectPath,
      [promise,
       pServer,
       asyncSystem,
       pAssetAccessor,
       friendlyApplicationName,
       clientOptions,
       tokenEndpointUrl,
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
          promise.resolve(Result<OAuth2TokenResponse>(
              ErrorList{{errorMessage, errorDescriptionMessage}, {}}));
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
          promise.resolve(Result<OAuth2TokenResponse>(
              ErrorList{{"Received an invalid state."}, {}}));
          return;
        }

        try {
          Result<OAuth2TokenResponse> tokenExchangeResult =
              completeTokenExchange(
                  asyncSystem,
                  pAssetAccessor,
                  clientOptions,
                  tokenEndpointUrl,
                  code,
                  redirectUrl,
                  codeVerifier)
                  .wait();

          if (!tokenExchangeResult.value.has_value()) {
            response.set_content(
                createGenericErrorHtml(
                    friendlyApplicationName,
                    "Failed to obtain token",
                    tokenExchangeResult.errors),
                "text/html");
          } else {
            response.set_content(
                createSuccessHtml(friendlyApplicationName),
                "text/html");
          }

          promise.resolve(std::move(tokenExchangeResult));
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

CesiumAsync::Future<Result<OAuth2TokenResponse>>
OAuth2PKCE::completeTokenExchange(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const OAuth2ClientOptions& clientOptions,
    const std::string& tokenEndpointUrl,
    const std::string& code,
    const std::string& redirectUrl,
    const std::string& codeVerifier) {
  std::string contentType = "application/json";
  std::span<const std::byte> payload;
  rapidjson::StringBuffer postBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(postBuffer);
  std::string queryStr;

  if (clientOptions.useJsonBody) {
    writer.StartObject();
    writer.Key("grant_type");
    writer.String("authorization_code");
    writer.Key("client_id");
    writer.String(clientOptions.clientID.c_str());
    writer.Key("code");
    writer.String(code.c_str(), rapidjson::SizeType(code.size()));
    writer.Key("redirect_uri");
    writer.String(redirectUrl.c_str(), rapidjson::SizeType(redirectUrl.size()));
    writer.Key("code_verifier");
    writer.String(
        codeVerifier.c_str(),
        rapidjson::SizeType(codeVerifier.size()));
    writer.EndObject();

    payload = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(postBuffer.GetString()),
        reinterpret_cast<const std::byte*>(
            postBuffer.GetString() + postBuffer.GetSize()));
  } else {
    CesiumUtility::UriQuery tokenEndpointQuery;
    tokenEndpointQuery.setValue("grant_type", "authorization_code");
    tokenEndpointQuery.setValue("client_id", clientOptions.clientID);
    tokenEndpointQuery.setValue("code", code);
    tokenEndpointQuery.setValue("redirect_uri", redirectUrl);
    tokenEndpointQuery.setValue("code_verifier", codeVerifier);
    queryStr = tokenEndpointQuery.toQueryString();
    payload = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(queryStr.data()),
        reinterpret_cast<const std::byte*>(queryStr.data() + queryStr.size()));

    contentType = "application/x-www-form-urlencoded";
  }

  return pAssetAccessor
      ->request(
          asyncSystem,
          "POST",
          tokenEndpointUrl,
          {{"Content-Type", contentType}, {"Accept", "application/json"}},
          payload)
      .thenInWorkerThread(
          [asyncSystem, pAssetAccessor](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
            const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return Result<OAuth2TokenResponse>(
                  ErrorList{{"The server did not return a response."}, {}});
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              std::string error, errorDesc;
              if (parseErrorResponse(pResponse->data(), error, errorDesc)) {
                return Result<OAuth2TokenResponse>(ErrorList::error(fmt::format(
                    "Received error '{}' while obtaining token: {}",
                    error,
                    errorDesc)));
              }

              return Result<OAuth2TokenResponse>(ErrorList{
                  {fmt::format(
                      "The server returned an error code: {}",
                      pResponse->statusCode())},
                  {}});
            }

            rapidjson::Document d;
            d.Parse(
                reinterpret_cast<const char*>(pResponse->data().data()),
                pResponse->data().size());
            if (d.HasParseError()) {
              return Result<OAuth2TokenResponse>(ErrorList{
                  {fmt::format(
                      "Failed to parse JSON response: {}",
                      rapidjson::GetParseError_En(d.GetParseError()))},
                  {}});
            }

            std::string accessToken =
                JsonHelpers::getStringOrDefault(d, "access_token", "");
            if (accessToken.empty()) {
              return Result<OAuth2TokenResponse>(ErrorList{
                  {"Server response does not include a valid token."},
                  {}});
            }

            std::optional<std::string> refreshToken;
            const auto& refreshTokenMember = d.FindMember("refresh_token");
            if (refreshTokenMember != d.MemberEnd() &&
                refreshTokenMember->value.IsString()) {
              refreshToken = refreshTokenMember->value.GetString();
            }

            return Result<OAuth2TokenResponse>(
                OAuth2TokenResponse{accessToken, refreshToken});
          });
}

CesiumAsync::Future<CesiumUtility::Result<OAuth2TokenResponse>>
OAuth2PKCE::refresh(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const OAuth2ClientOptions& clientOptions,
    const std::string& refreshBaseUrl,
    const std::string& refreshToken) {
  const std::string redirectUrl =
      clientOptions.redirectPort
          ? Uri::resolve(
                "http://127.0.0.1:" +
                    std::to_string(*clientOptions.redirectPort),
                clientOptions.redirectPath)
          : Uri::resolve("http://127.0.0.1", clientOptions.redirectPath);

  std::string contentType = "application/json";
  std::span<const std::byte> payload;
  rapidjson::StringBuffer postBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(postBuffer);
  std::string postBody;

  if (clientOptions.useJsonBody) {
    writer.StartObject();
    writer.Key("grant_type");
    writer.String("refresh_token");
    writer.Key("client_id");
    writer.String(clientOptions.clientID.c_str());
    writer.Key("redirect_uri");
    writer.String(redirectUrl.c_str(), rapidjson::SizeType(redirectUrl.size()));
    writer.Key("refresh_token");
    writer.String(
        refreshToken.c_str(),
        rapidjson::SizeType(refreshToken.size()));
    writer.EndObject();

    payload = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(postBuffer.GetString()),
        postBuffer.GetSize());
  } else {
    CesiumUtility::UriQuery refreshEndpointQuery;
    refreshEndpointQuery.setValue("grant_type", "refresh_token");
    refreshEndpointQuery.setValue("client_id", clientOptions.clientID);
    refreshEndpointQuery.setValue("redirect_uri", redirectUrl);
    refreshEndpointQuery.setValue("refresh_token", refreshToken);
    postBody = refreshEndpointQuery.toQueryString();
    payload = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(postBody.data()),
        postBody.size());
    contentType = "application/x-www-form-urlencoded";
  }

  return pAssetAccessor
      ->request(
          asyncSystem,
          "POST",
          refreshBaseUrl,
          {{"Content-Type", contentType}, {"Accept", "application/json"}},
          payload)
      .thenInWorkerThread(
          [asyncSystem, pAssetAccessor](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
            const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return Result<OAuth2TokenResponse>(
                  ErrorList{{"The server did not return a response."}, {}});
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              std::string error, errorDesc;
              if (parseErrorResponse(pResponse->data(), error, errorDesc)) {
                return Result<OAuth2TokenResponse>(ErrorList::error(fmt::format(
                    "Received error '{}' while refreshing token: {}",
                    error,
                    errorDesc)));
              }

              return Result<OAuth2TokenResponse>(ErrorList{
                  {fmt::format(
                      "The server returned an error code: {}",
                      pResponse->statusCode())},
                  {}});
            }

            rapidjson::Document d;
            d.Parse(
                reinterpret_cast<const char*>(pResponse->data().data()),
                pResponse->data().size());
            if (d.HasParseError()) {
              return Result<OAuth2TokenResponse>(ErrorList{
                  {fmt::format(
                      "Failed to parse JSON response: {}",
                      rapidjson::GetParseError_En(d.GetParseError()))},
                  {}});
            }

            std::string accessToken =
                JsonHelpers::getStringOrDefault(d, "access_token", "");
            if (accessToken.empty()) {
              return Result<OAuth2TokenResponse>(ErrorList{
                  {"Server response does not include a valid access token."},
                  {}});
            }

            std::string refreshToken =
                JsonHelpers::getStringOrDefault(d, "refresh_token", "");
            if (accessToken.empty()) {
              return Result<OAuth2TokenResponse>(ErrorList{
                  {"Server response does not include a valid refresh token."},
                  {}});
            }

            return Result<OAuth2TokenResponse>(
                OAuth2TokenResponse{accessToken, refreshToken});
          });
}
} // namespace CesiumClientCommon
