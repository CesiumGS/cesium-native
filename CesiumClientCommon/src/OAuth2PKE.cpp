#include "fillWithRandomBytes.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumClientCommon/OAuth2PKE.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>

#include <httplib.h>
#include <modp_b64.h>
#include <picosha2.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <string>
#include <vector>

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

CesiumAsync::Future<Result<OAuth2TokenResponse>> OAuth2PKE::authorize(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& friendlyApplicationName,
    const std::string& clientID,
    const std::string& redirectPath,
    const std::vector<std::string>& scopes,
    std::function<void(const std::string&)>&& openUrlCallback,
    const std::string& tokenEndpointUrl,
    const std::string& authorizeBaseUrl) {
  auto promise = asyncSystem.createPromise<Result<OAuth2TokenResponse>>();

  std::shared_ptr<httplib::Server> pServer =
      std::make_shared<httplib::Server>();
  const int port = pServer->bind_to_any_port("127.0.0.1");

  std::string redirectUrl =
      Uri::resolve("http://127.0.0.1:" + std::to_string(port), redirectPath);

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
  authorizeUriQuery.setValue("client_id", clientID);
  authorizeUriQuery.setValue("scope", joinToString(scopes, " "));
  authorizeUriQuery.setValue("redirect_uri", redirectUrl);
  authorizeUriQuery.setValue("state", state);
  authorizeUriQuery.setValue("code_challenge_method", "S256");
  authorizeUriQuery.setValue("code_challenge", hashedChallenge);
  authorizeUri.setQuery(authorizeUriQuery.toQueryString());

  const std::string authorizeUrl = std::string(authorizeUri.toString());

  pServer->Get(
      redirectPath,
      [promise,
       pServer,
       asyncSystem,
       pAssetAccessor,
       friendlyApplicationName,
       clientID,
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
                  clientID,
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
OAuth2PKE::completeTokenExchange(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& clientID,
    const std::string& tokenEndpointUrl,
    const std::string& code,
    const std::string& redirectUrl,
    const std::string& codeVerifier) {
  rapidjson::StringBuffer postBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(postBuffer);

  writer.StartObject();
  writer.Key("grant_type");
  writer.String("authorization_code");
  writer.Key("client_id");
  writer.String(clientID.c_str());
  writer.Key("code");
  writer.String(code.c_str(), rapidjson::SizeType(code.size()));
  writer.Key("redirect_uri");
  writer.String(redirectUrl.c_str(), rapidjson::SizeType(redirectUrl.size()));
  writer.Key("code_verifier");
  writer.String(codeVerifier.c_str(), rapidjson::SizeType(codeVerifier.size()));
  writer.EndObject();

  const std::span<const std::byte> payload(
      reinterpret_cast<const std::byte*>(postBuffer.GetString()),
      postBuffer.GetSize());

  return pAssetAccessor
      ->request(
          asyncSystem,
          "POST",
          tokenEndpointUrl,
          {{"Content-Type", "application/json"},
           {"Accept", "application/json"}},
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
} // namespace CesiumClientCommon