#include "CesiumIonClient/CesiumIonConnection.h"
#include "CesiumIonClient/CesiumIonProfile.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumUtility/Uri.h"
#include "CesiumUtility/JsonHelpers.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

using namespace CesiumIonClient;
using namespace CesiumAsync;
using namespace CesiumUtility;

template <typename T>
static CesiumIonConnection::Response<T> createErrorResponse(const IAssetResponse* pResponse) {
    return CesiumIonConnection::Response<T> {
        std::nullopt,
        pResponse->statusCode(),
        "TODO",
        "Fill in this message"
    };
}

CesiumAsync::Future<CesiumIonConnection::Response<CesiumIonConnection>> CesiumIonConnection::connect(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& username,
    const std::string& password,
    const std::string& apiUrl
) {
    rapidjson::StringBuffer loginBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(loginBuffer);
    
    writer.StartObject();
    writer.Key("username");
    writer.String(username.c_str(), rapidjson::SizeType(username.size()));
    writer.Key("password");
    writer.String(password.c_str(), rapidjson::SizeType(password.size()));
    writer.EndObject();

    gsl::span<const uint8_t> loginBytes(reinterpret_cast<const uint8_t*>(loginBuffer.GetString()), loginBuffer.GetSize());

    return asyncSystem.post(
        Uri::resolve(apiUrl, "signIn"),
        { {"Content-Type", "application/json"} },
        loginBytes
    ).thenInMainThread([asyncSystem](std::unique_ptr<IAssetRequest>&& pRequest) -> Response<CesiumIonConnection> {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            return Response<CesiumIonConnection> {
                std::nullopt,
                0,
                "NoResponse",
                "The server did not return a response."
            };
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            return createErrorResponse<CesiumIonConnection>(pResponse);
        }

        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());
        if (d.HasParseError()) {
            return Response<CesiumIonConnection> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                std::string("Failed to parse JSON response: ") + rapidjson::GetParseError_En(d.GetParseError())
            };
        }

        if (!d.IsObject()) {
            return Response<CesiumIonConnection> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                "Response is not a JSON object."
            };
        }

        auto tokenIt = d.FindMember("token");
        if (tokenIt == d.MemberEnd()) {
            return Response<CesiumIonConnection> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                "Response is missing the expected \"token\" property."
            };
        }

        return Response<CesiumIonConnection> {
            CesiumIonConnection(asyncSystem, tokenIt->value.GetString()),
            pResponse->statusCode(),
            std::string(),
            std::string()
        };
    });
}

CesiumIonConnection::CesiumIonConnection(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& accessToken,
    const std::string& apiUrl
) :
    _asyncSystem(asyncSystem),
    _accessToken(accessToken),
    _apiUrl(apiUrl)
{
}

CesiumAsync::Future<CesiumIonConnection::Response<CesiumIonProfile>> CesiumIonConnection::me() const {
    return this->_asyncSystem.requestAsset(
        CesiumUtility::Uri::resolve(this->_apiUrl, "v1/me")
    ).thenInMainThread([](std::unique_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            return Response<CesiumIonProfile> {
                std::nullopt,
                0,
                "NoResponse",
                "The server did not return a response."
            };
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            return createErrorResponse<CesiumIonProfile>(pResponse);
        }

        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());
        if (d.HasParseError()) {
            return Response<CesiumIonProfile> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                std::string("Failed to parse JSON response: ") + rapidjson::GetParseError_En(d.GetParseError())
            };
        }

        if (!d.IsObject()) {
            return Response<CesiumIonProfile> {
                std::nullopt,
                pResponse->statusCode(),
                "ParseError",
                "Response is not a JSON object."
            };
        }

        CesiumIonProfile result;

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

        return Response<CesiumIonProfile> {
            result,
            pResponse->statusCode(),
            std::string(),
            std::string()
        };
    });
}
