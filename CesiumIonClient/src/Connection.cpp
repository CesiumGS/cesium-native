#include "CesiumAsync/IAssetResponse.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumUtility/joinToString.h"
#include "CesiumUtility/JsonHelpers.h"
#include "CesiumUtility/Uri.h"
#include <duthomhas/csprng.hpp>
#include <httplib.h>
#include <modp_b64.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <thread>

#pragma warning(push)
#pragma warning(disable:4244)
#include <picosha2.h>
#pragma warning(pop)

using namespace CesiumAsync;
using namespace CesiumIonClient;
using namespace CesiumUtility;

namespace {
    std::string encodeBase64(const std::vector<uint8_t>& bytes) {
        size_t count = modp_b64_encode_len(bytes.size());
        std::string result(count, 0);
        size_t actualLength = modp_b64_encode(result.data(), reinterpret_cast<const char*>(bytes.data()), bytes.size());
        result.resize(actualLength);
        
        // Convert to a URL-friendly form of Base64 according to the algorithm
        // in [RFC7636 Appendix A](https://tools.ietf.org/html/rfc7636#appendix-A)
        size_t firstPaddingIndex = result.find('=');
        if (firstPaddingIndex != std::string::npos) {
            result.erase(result.begin() + int64_t(firstPaddingIndex), result.end());
        }
        std::replace(result.begin(), result.end(), '+', '-');
        std::replace(result.begin(), result.end(), '/', '_');

        return result;
    }
}

/*static*/ CesiumAsync::Future<Connection> Connection::authorize(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& friendlyApplicationName,
    int64_t clientID,
    const std::string& redirectPath,
    const std::vector<std::string>& scopes,
    std::function<void (const std::string&)>&& openUrlCallback,
    const std::string& ionApiUrl,
    const std::string& ionAuthorizeUrl
) {
    return asyncSystem.createFuture<Connection>([
        &asyncSystem,
        &pAssetAccessor,
        &friendlyApplicationName,
        &clientID,
        &redirectPath,
        &scopes,
        &ionApiUrl,
        &ionAuthorizeUrl,
        &openUrlCallback
    ](auto& promise) {
        std::shared_ptr<httplib::Server> pServer = std::make_shared<httplib::Server>();
        int port = pServer->bind_to_any_port("127.0.0.1");

        std::string redirectUrl = Uri::resolve("http://127.0.0.1:" + std::to_string(port), redirectPath);

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
        authorizeUrl = Uri::addQuery(authorizeUrl, "client_id", std::to_string(clientID));
        authorizeUrl = Uri::addQuery(authorizeUrl, "scope", joinToString(scopes, " "));
        authorizeUrl = Uri::addQuery(authorizeUrl, "redirect_uri", redirectUrl);
        authorizeUrl = Uri::addQuery(authorizeUrl, "state", state);
        authorizeUrl = Uri::addQuery(authorizeUrl, "code_challenge_method", "S256");
        authorizeUrl = Uri::addQuery(authorizeUrl, "code_challenge", hashedChallenge);

        // TODO: state and code_challenge

        pServer->Get(redirectPath.c_str(), redirectPath.size(), [
            promise,
            pServer,
            asyncSystem,
            pAssetAccessor,
            friendlyApplicationName,
            clientID,
            ionApiUrl,
            redirectUrl,
            expectedState = state,
            codeVerifier
        ](const httplib::Request& request, httplib::Response& response) {
            pServer->stop();
            
            std::string code = request.get_param_value("code");
            std::string state = request.get_param_value("state");

            if (state != expectedState) {
                response.set_content("Invalid state! Please close this window and return to " + friendlyApplicationName + " to try again.", "text/html");
                promise.reject(std::runtime_error("Received an invalid state."));
                return;
            }

            std::variant<Connection, std::exception> result = Connection::completeTokenExchange(asyncSystem, pAssetAccessor, clientID, ionApiUrl, code, redirectUrl, codeVerifier).wait();

            // TODO: nicer HTML

            struct Operation {
                const std::string& friendlyApplicationName;
                httplib::Response& response;
                const AsyncSystem::Promise<Connection>& promise;

                void operator()(Connection& connection) {
                    response.set_content("Successfully authorized! Please close this window and return to " + friendlyApplicationName + ".", "text/html");
                    promise.resolve(std::move(connection));
                }

                void operator()(std::exception& exception) {
                    response.set_content("Not authorized! Please close this window and return to " + friendlyApplicationName + ".", "text/html");
                    promise.reject(std::move(exception));
                }
            };

            std::visit(Operation { friendlyApplicationName, response, promise }, result);
        });

        // TODO: Make this process cancelable, and shut down the server when it's canceled.
        std::thread([pServer, authorizeUrl]() {
            pServer->listen_after_bind();
        }).detach();

        openUrlCallback(authorizeUrl);
    });
}

Connection::Connection(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& accessToken,
    const std::string& apiUrl
) :
    _asyncSystem(asyncSystem),
    _pAssetAccessor(pAssetAccessor),
    _accessToken(accessToken),
    _apiUrl(apiUrl)
{
}

template <typename T>
static Response<T> createErrorResponse(const IAssetResponse* pResponse) {
    return Response<T> {
        std::nullopt,
        pResponse->statusCode(),
        "TODO",
        "Fill in this message"
    };
}

CesiumAsync::Future<Response<Profile>> Connection::me() const {
    return this->_pAssetAccessor->requestAsset(
        this->_asyncSystem,
        CesiumUtility::Uri::resolve(this->_apiUrl, "v1/me"),
        {
            {"Accept", "application/json"},
            {"Authorization", "Bearer " + this->_accessToken}
        }
    ).thenInMainThread([](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            return Response<Profile> {
                std::nullopt,
                0,
                "NoResponse",
                "The server did not return a response."
            };
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            return createErrorResponse<Profile>(pResponse);
        }

        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());
        if (d.HasParseError()) {
            return Response<Profile> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                std::string("Failed to parse JSON response: ") + rapidjson::GetParseError_En(d.GetParseError())
            };
        }

        if (!d.IsObject()) {
            return Response<Profile> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                "Response is not a JSON object."
            };
        }

        Profile result;

        result.id = JsonHelpers::getInt64OrDefault(d, "id", -1);
        result.scopes = JsonHelpers::getStrings(d, "scopes");
        result.username = JsonHelpers::getStringOrDefault(d, "username", "");
        result.email = JsonHelpers::getStringOrDefault(d, "email", "");
        result.emailVerified = JsonHelpers::getBoolOrDefault(d, "emailVerified", false);
        result.avatar = JsonHelpers::getStringOrDefault(d, "avatar", "");
        
        auto storageIt = d.FindMember("storage");
        if (storageIt == d.MemberEnd()) {
            result.storage.available = 0;
            result.storage.total = 0;
            result.storage.used = 0;
        } else {
            const rapidjson::Value& storage = storageIt->value;
            result.storage.available = JsonHelpers::getInt64OrDefault(storage, "available", 0);
            result.storage.total = JsonHelpers::getInt64OrDefault(storage, "total", 0);
            result.storage.used = JsonHelpers::getInt64OrDefault(storage, "used", 0);
        }

        return Response<Profile> {
            result,
            pResponse->statusCode(),
            std::string(),
            std::string()
        };
    });
}

CesiumAsync::Future<Response<Assets>> Connection::assets() const {
    return this->_pAssetAccessor->requestAsset(
        this->_asyncSystem,
        CesiumUtility::Uri::resolve(this->_apiUrl, "v1/assets"),
        {
            {"Accept", "application/json"},
            {"Authorization", "Bearer " + this->_accessToken}
        }
    ).thenInMainThread([](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            return Response<Assets> {
                std::nullopt,
                0,
                "NoResponse",
                "The server did not return a response."
            };
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            return createErrorResponse<Assets>(pResponse);
        }

        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());
        if (d.HasParseError()) {
            return Response<Assets> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                std::string("Failed to parse JSON response: ") + rapidjson::GetParseError_En(d.GetParseError())
            };
        }

        if (!d.IsObject()) {
            return Response<Assets> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                "Response is not a JSON object."
            };
        }

        Assets result;

        result.link = JsonHelpers::getStringOrDefault(d, "link", "");
        
        auto itemsIt = d.FindMember("items");
        if (itemsIt != d.MemberEnd() && itemsIt->value.IsArray()) {
            const rapidjson::Value& items = itemsIt->value;
            result.items.resize(items.Size());
            std::transform(items.Begin(), items.End(), result.items.begin(), [](const rapidjson::Value& item) {
                Asset result;
                result.id = JsonHelpers::getInt64OrDefault(item, "id", -1);
                result.name = JsonHelpers::getStringOrDefault(item, "name", "");
                result.description = JsonHelpers::getStringOrDefault(item, "description", "");
                result.attribution = JsonHelpers::getStringOrDefault(item, "attribution", "");
                result.type = JsonHelpers::getStringOrDefault(item, "type", "");
                result.bytes = JsonHelpers::getInt64OrDefault(item, "bytes", -1);
                result.dateAdded = JsonHelpers::getStringOrDefault(item, "dateAdded", "");
                result.status = JsonHelpers::getStringOrDefault(item, "status", "");
                result.percentComplete = int8_t(JsonHelpers::getInt32OrDefault(item, "percentComplete", -1));
                return result;
            });
        }

        return Response<Assets> {
            result,
            pResponse->statusCode(),
            std::string(),
            std::string()
        };
    });
}

static std::optional<Token> tokenFromJson(const rapidjson::Value& json) {
    if (!json.IsObject()) {
        return std::nullopt;
    }

    Token token;
    token.jti = JsonHelpers::getStringOrDefault(json, "jti", "");
    token.name = JsonHelpers::getStringOrDefault(json, "name", "");
    token.token = JsonHelpers::getStringOrDefault(json, "token", "");
    token.isDefault = JsonHelpers::getBoolOrDefault(json, "isDefault", false);
    token.lastUsed = JsonHelpers::getStringOrDefault(json, "lastUsed", "");
    token.scopes = JsonHelpers::getStrings(json, "scopes");

    auto assetsIt = json.FindMember("assets");
    if (assetsIt != json.MemberEnd()) {
        token.assets = JsonHelpers::getInt64s(json, "assets");
    } else {
        token.assets = std::nullopt;
    }

    return token;
}

CesiumAsync::Future<Response<std::vector<Token>>> Connection::tokens() const {
    return this->_pAssetAccessor->requestAsset(
        this->_asyncSystem,
        CesiumUtility::Uri::resolve(this->_apiUrl, "v1/tokens"),
        {
            {"Accept", "application/json"},
            {"Authorization", "Bearer " + this->_accessToken}
        }
    ).thenInMainThread([](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            return Response<std::vector<Token>> {
                std::nullopt,
                0,
                "NoResponse",
                "The server did not return a response."
            };
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            return createErrorResponse<std::vector<Token>>(pResponse);
        }

        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());
        if (d.HasParseError()) {
            return Response<std::vector<Token>> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                std::string("Failed to parse JSON response: ") + rapidjson::GetParseError_En(d.GetParseError())
            };
        }

        if (!d.IsArray()) {
            return Response<std::vector<Token>> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                "Response is not a JSON array."
            };
        }

        std::vector<Token> result;

        for (auto it = d.Begin(); it != d.End(); ++it) {
            std::optional<Token> token = tokenFromJson(*it);
            if (!token) {
                continue;
            }

            result.emplace_back(std::move(token.value()));
        }

        return Response<std::vector<Token>> {
            result,
            pResponse->statusCode(),
            std::string(),
            std::string()
        };
    });
}

CesiumAsync::Future<Response<Token>> Connection::createToken(
    const std::string& name,
    const std::vector<std::string>& scopes,
    const std::optional<std::vector<int64_t>>& assets
) const {
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
    writer.Key("assets");
    if (assets) {
        writer.StartArray();
        for (auto asset : assets.value()) {
            writer.Int64(asset);
        }
        writer.EndArray();
    } else {
        writer.Null();
    }
    writer.EndObject();

    gsl::span<const uint8_t> tokenBytes(reinterpret_cast<const uint8_t*>(tokenBuffer.GetString()), tokenBuffer.GetSize());
    return this->_pAssetAccessor->post(
        this->_asyncSystem,
        CesiumUtility::Uri::resolve(this->_apiUrl, "v1/tokens"),
        {
            {"Content-Type", "application/json"},
            {"Accept", "application/json"},
            {"Authorization", "Bearer " + this->_accessToken}
        },
        tokenBytes
    ).thenInMainThread([](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            return Response<Token> {
                std::nullopt,
                0,
                "NoResponse",
                "The server did not return a response."
            };
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            return createErrorResponse<Token>(pResponse);
        }

        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());
        if (d.HasParseError()) {
            return Response<Token> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                std::string("Failed to parse JSON response: ") + rapidjson::GetParseError_En(d.GetParseError())
            };
        }

        std::optional<Token> maybeToken = tokenFromJson(d);

        if (!maybeToken) {
            return Response<Token> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                "Response is not a JSON object."
            };
        }

        return Response<Token> {
            std::move(maybeToken),
            pResponse->statusCode(),
            std::string(),
            std::string()
        };
    });
}

/*static*/ CesiumAsync::Future<Connection> Connection::completeTokenExchange(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    int64_t clientID,
    const std::string& ionApiUrl,
    const std::string& code,
    const std::string& redirectUrl,
    const std::string& codeVerifier
) {
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

    gsl::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(postBuffer.GetString()), postBuffer.GetSize());

    return pAssetAccessor->post(
        asyncSystem,
        Uri::resolve(ionApiUrl, "oauth/token"),
        {
            {"Content-Type", "application/json"},
            {"Accept", "application/json"}
        },
        payload
    ).thenInWorkerThread([
        asyncSystem,
        pAssetAccessor,
        ionApiUrl
    ](std::shared_ptr<IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            throw std::runtime_error("The server did not return a response.");
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            throw std::runtime_error("The server returned an error code: " + std::to_string(pResponse->statusCode()));
        }

        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());
        if (d.HasParseError()) {
            throw std::runtime_error(std::string("Failed to parse JSON response: ") + rapidjson::GetParseError_En(d.GetParseError()));
        }

        std::string accessToken = JsonHelpers::getStringOrDefault(d, "access_token", "");
        if (accessToken.empty()) {
            throw std::runtime_error("Server response does not include a valid token.");
        }

        return Connection(
            asyncSystem,
            pAssetAccessor,
            accessToken,
            ionApiUrl
        );
    });
}
