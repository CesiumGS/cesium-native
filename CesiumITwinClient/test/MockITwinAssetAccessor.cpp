#include "MockITwinAssetAccessor.h"

#include "CesiumNativeTests/SimpleAssetRequest.h"
#include "CesiumUtility/Uri.h"

#include <CesiumClientCommon/fillWithRandomBytes.h>

#include <modp_b64.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <chrono>

namespace {
void writeMap(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    const std::unordered_map<std::string, std::string>& map) {
  for (const auto& [k, v] : map) {
    writer.Key(k.c_str());
    writer.String(v.c_str());
  }
}

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
} // namespace

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

  const std::chrono::time_point nbf =
      std::chrono::system_clock::now() - std::chrono::minutes(5);
  const std::chrono::time_point exp =
      std::chrono::system_clock::now() + std::chrono::hours(1);

  writer.StartObject();
  writer.Key("scope");
  writer.StartArray();
  writer.String("itwin-platform");
  writer.String("offline_access");
  writer.EndArray();
  writer.Key("name");
  writer.String("Example.User@example.com");
  writer.Key("preferred_username");
  writer.String("Example.User@example.com");
  writer.Key("nbf");
  writer.Int64(
      std::chrono::duration_cast<std::chrono::seconds>(nbf.time_since_epoch())
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

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
MockITwinAssetAccessor::get(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  return this
      ->request(asyncSystem, "GET", url, headers, std::span<const std::byte>());
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
MockITwinAssetAccessor::request(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const std::span<const std::byte>& body) {
  std::vector<std::byte> bodyBuffer(body.data(), body.data() + body.size());
  CesiumUtility::Uri uri(url);

  if (uri.getHost() == "ims.bentley.com") {
    return handleAuthServer(asyncSystem, verb, uri, headers, bodyBuffer);
  } else if (uri.getHost() == "api.bentley.com") {
    return handleApiServer(asyncSystem, verb, uri, headers, bodyBuffer);
  }

  FAIL("Cannot find request for url " << url);

  return asyncSystem.createResolvedFuture(
      std::shared_ptr<CesiumAsync::IAssetRequest>(nullptr));
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
MockITwinAssetAccessor::handleAuthServer(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const CesiumUtility::Uri& uri,
    const std::vector<THeader>& /*headers*/,
    const std::vector<std::byte>& body) {
  const std::string bodyStr(
      reinterpret_cast<const char*>(body.data()),
      body.size());
  CesiumUtility::UriQuery bodyParams((std::string_view(bodyStr)));

  rapidjson::StringBuffer outputBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(outputBuffer);

  writer.StartObject();
  if (uri.getPath() == "/connect/token") {
    REQUIRE(bodyParams.getValue("grant_type"));

    const std::string_view grantType = *bodyParams.getValue("grant_type");
    const bool grantTypeCheck =
        grantType == "authorization_code" || grantType == "refresh_token";
    CHECK(grantTypeCheck);

    if (grantType == "refresh_token") {
      REQUIRE(this->refreshToken);
      CHECK(bodyParams.getValue("refresh_token") == this->refreshToken);
    }

    this->authToken = generateAuthToken();
    this->refreshToken = randomStringOfLen(42);

    writer.Key("access_token");
    writer.String(this->authToken.c_str());
    writer.Key("refresh_token");
    writer.String(this->refreshToken->c_str());
    writer.Key("token_type");
    writer.String("Bearer");
    writer.Key("expires_in");
    writer.Int(3599);
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

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
MockITwinAssetAccessor::handleApiServer(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const CesiumUtility::Uri& uri,
    const std::vector<THeader>& headers,
    const std::vector<std::byte>& body) {
  const auto& authIt =
      std::find_if(headers.begin(), headers.end(), [](const THeader& header) {
        return header.first == "Authorization";
      });

  REQUIRE(authIt != headers.end());
  REQUIRE(authIt->second.starts_with("Bearer "));

  const std::string& headerToken = authIt->second.substr(strlen("Bearer "));
  CHECK(headerToken == this->authToken);

  const std::string bodyStr(
      reinterpret_cast<const char*>(body.data()),
      body.size());
  CesiumUtility::UriQuery bodyParams((std::string_view(bodyStr)));

  rapidjson::StringBuffer outputBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(outputBuffer);

  writer.StartObject();
  if (uri.getPath() == "/users/me") {
    writer.Key("user");
    writer.StartObject();
    writeMap(
        writer,
        std::unordered_map<std::string, std::string>{
            {"id", "00000000-0000-0000-0000-000000000000"},
            {"displayName", "John.Smith@example.com"},
            {"givenName", "John"},
            {"surname", "Smith"},
            {"email", "John.Smith@example.com"},
            {"alternateEmail", "John.Smith@example.com"},
            {"phone", "000-000-0000"},
            {"organizationName", "Example Organization"},
            {"city", "Anytown"},
            {"country", "US"},
            {"language", "EN"},
            {"createdDateTime", "2020-03-25T04,36,40.4210000Z"}});
    writer.EndObject();
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