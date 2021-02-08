#include "CesiumIonClient/CesiumIonConnection.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumUtility/Uri.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace CesiumIonClient;
using namespace CesiumAsync;
using namespace CesiumUtility;

CesiumAsync::Future<std::optional<CesiumIonConnection>> CesiumIonConnection::connect(
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

    return asyncSystem
        .post(Uri::resolve(apiUrl, "signIn"), { {"Content-Type", "application/json"} }, loginBytes)
        .thenInMainThread([asyncSystem](std::unique_ptr<IAssetRequest>&& pRequest) -> std::optional<CesiumIonConnection> {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
                return std::nullopt;
            }

            if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
                return std::nullopt;
            }

            rapidjson::Document d;
            d.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());
            if (d.HasParseError()) {
                return std::nullopt;
            }

            if (!d.IsObject()) {
                return std::nullopt;
            }

            auto tokenIt = d.FindMember("token");
            if (tokenIt == d.MemberEnd()) {
                return std::nullopt;
            }

            return CesiumIonConnection(asyncSystem, tokenIt->value.GetString());
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
