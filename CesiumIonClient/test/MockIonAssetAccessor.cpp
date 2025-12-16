#include "MockIonAssetAccessor.h"

#include <CesiumClientCommon/fillWithRandomBytes.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/Uri.h>

#include <modp_b64.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <chrono>
#include <filesystem>

namespace {
std::string encodeBase64(const std::string& str) {
  const size_t len = modp_b64_encode_len(str.size());
  std::string output;
  output.resize(len);
  const size_t bytes = modp_b64_encode(output.data(), str.data(), str.size());
  REQUIRE(bytes != -1);
  output.resize(bytes);
  return output;
}

const char* ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvxyz0123456789";

std::string randomStringOfLen(size_t len) {
  std::string str;
  str.reserve(len);

  std::vector<uint8_t> buffer(len, 0);
  CesiumClientCommon::fillWithRandomBytes(buffer);

  const size_t alphabetLen = strlen(ALPHABET);
  for (size_t i = 0; i < len; i++) {
    str += ALPHABET[buffer[i] % alphabetLen];
  }

  return str;
}

std::string generateAuthToken() {
  rapidjson::StringBuffer tokenBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(tokenBuffer);

  const std::chrono::time_point exp =
      std::chrono::system_clock::now() + std::chrono::minutes(15);

  writer.StartObject();
  writer.Key("id");
  writer.Int64(222);
  writer.Key("iat");
  writer.Int64(std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count());
  writer.Key("exp");
  writer.Int64(
      std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch())
          .count());
  writer.EndObject();

  std::string tokenJson(tokenBuffer.GetString(), tokenBuffer.GetSize());
  return randomStringOfLen(74) + "." + encodeBase64(tokenJson) + "." +
         randomStringOfLen(342);
}
} // namespace

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
MockIonAssetAccessor::get(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  return this
      ->request(asyncSystem, "GET", url, headers, std::span<const std::byte>());
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
MockIonAssetAccessor::request(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const std::span<const std::byte>& body) {
  std::vector<std::byte> bodyBuffer(body.data(), body.data() + body.size());
  CesiumUtility::Uri uri(url);

  return handleApiServer(asyncSystem, verb, uri, headers, bodyBuffer);
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
MockIonAssetAccessor::handleApiServer(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const CesiumUtility::Uri& uri,
    const std::vector<THeader>& headers,
    const std::vector<std::byte>& bodyBytes) {
  rapidjson::StringBuffer outputBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(outputBuffer);

  writer.StartObject();
  if (uri.getPath() == "/oauth/token") {
    rapidjson::Document body;
    body.Parse(
        reinterpret_cast<const char*>(bodyBytes.data()),
        bodyBytes.size());
    REQUIRE(!body.HasParseError());
    REQUIRE(body.IsObject());

    const rapidjson::Value::Object& bodyObj = body.GetObject();
    REQUIRE(bodyObj.FindMember("grant_type") != bodyObj.MemberEnd());

    const std::string_view grantType =
        bodyObj.FindMember("grant_type")->value.GetString();
    const bool grantTypeCheck =
        grantType == "authorization_code" || grantType == "refresh_token";
    CHECK(grantTypeCheck);

    if (grantType == "refresh_token") {
      REQUIRE(this->refreshToken);
      REQUIRE(bodyObj.FindMember("refresh_token") != bodyObj.MemberEnd());
      CHECK(
          bodyObj.FindMember("refresh_token")->value.GetString() ==
          this->refreshToken);
    }

    this->authToken = generateAuthToken();
    this->refreshToken = randomStringOfLen(42);

    writer.Key("access_token");
    writer.String(this->authToken.c_str());
    writer.Key("refresh_token");
    writer.String(this->refreshToken->c_str());
    writer.Key("token_type");
    writer.String("bearer");
    writer.Key("expires_in");
    writer.Int(15 * 60 * 60);
    writer.Key("refresh_token_expires_in");
    writer.Int(45 * 60 * 60);
  } else if (uri.getPath() == "/v1/me") {
    const auto& authIt =
        std::find_if(headers.begin(), headers.end(), [](const THeader& header) {
          return header.first == "Authorization";
        });

    REQUIRE(authIt != headers.end());
    if (authIt->second.starts_with("Bearer ")) {
      const std::string& headerToken = authIt->second.substr(strlen("Bearer "));
      CHECK(headerToken == this->authToken);
    } else {
      FAIL("No auth token found");
    }

    return asyncSystem
        .createResolvedFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
            std::make_shared<CesiumNativeTests::SimpleAssetRequest>(
                verb,
                std::string(uri.toString()),
                CesiumAsync::HttpHeaders{},
                std::make_unique<CesiumNativeTests::SimpleAssetResponse>(
                    200,
                    "application/json",
                    CesiumAsync::HttpHeaders{},
                    readFile(
                        std::filesystem::path(CesiumIonClient_TEST_DATA_DIR) /
                        "profile.json"))));
  }
  writer.EndObject();

  return asyncSystem.createResolvedFuture<
      std::shared_ptr<CesiumAsync::IAssetRequest>>(
      std::make_shared<CesiumNativeTests::SimpleAssetRequest>(
          verb,
          std::string(uri.toString()),
          CesiumAsync::HttpHeaders{},
          std::make_unique<CesiumNativeTests::SimpleAssetResponse>(
              200,
              "application/json",
              CesiumAsync::HttpHeaders{},
              std::vector<std::byte>(
                  reinterpret_cast<const std::byte*>(outputBuffer.GetString()),
                  reinterpret_cast<const std::byte*>(
                      outputBuffer.GetString() + outputBuffer.GetSize())))));
}
MockIonAssetAccessor::MockIonAssetAccessor()
    : authToken(generateAuthToken()), refreshToken(randomStringOfLen(42)) {}
