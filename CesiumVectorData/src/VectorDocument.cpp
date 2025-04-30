#include "GeoJsonParser.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/VectorDocument.h>
#include <CesiumVectorData/VectorNode.h>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumVectorData {

Result<IntrusivePointer<VectorDocument>> VectorDocument::fromGeoJson(
    const std::span<const std::byte>& bytes,
    std::vector<VectorDocumentAttribution>&& attributions) {
  Result<VectorNode> parseResult = parseGeoJson(bytes);
  if (!parseResult.value) {
    return Result<IntrusivePointer<VectorDocument>>(
        std::move(parseResult.errors));
  }

  IntrusivePointer<VectorDocument> pDocument;
  pDocument.emplace(std::move(*parseResult.value), std::move(attributions));

  return Result<IntrusivePointer<VectorDocument>>(
      pDocument,
      std::move(parseResult.errors));
}

Result<IntrusivePointer<VectorDocument>> VectorDocument::fromGeoJson(
    const rapidjson::Document& document,
    std::vector<VectorDocumentAttribution>&& attributions) {
  Result<VectorNode> parseResult = parseGeoJson(document);
  if (!parseResult.value) {
    return Result<IntrusivePointer<VectorDocument>>(
        std::move(parseResult.errors));
  }

  IntrusivePointer<VectorDocument> pDocument;
  pDocument.emplace(std::move(*parseResult.value), std::move(attributions));

  return Result<IntrusivePointer<VectorDocument>>(
      pDocument,
      std::move(parseResult.errors));
}

Future<Result<IntrusivePointer<VectorDocument>>>
VectorDocument::fromCesiumIonAsset(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl) {
  const std::string url = fmt::format(
      "{}v1/assets/{}/endpoint?access_token={}",
      ionAssetEndpointUrl,
      ionAssetID,
      ionAccessToken);
  return pAssetAccessor->get(asyncSystem, url)
      .thenImmediately([asyncSystem, pAssetAccessor](
                           std::shared_ptr<IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
          return asyncSystem.createResolvedFuture<
              Result<IntrusivePointer<VectorDocument>>>(Result<IntrusivePointer<
                                                            VectorDocument>>(
              ErrorList::error(fmt::format(
                  "Status code {} while requesting Cesium ion vector asset.",
                  pResponse->statusCode()))));
        }

        rapidjson::Document response;
        response.Parse(
            reinterpret_cast<const char*>(pResponse->data().data()),
            pResponse->data().size());

        if (response.HasParseError()) {
          return asyncSystem
              .createResolvedFuture<Result<IntrusivePointer<VectorDocument>>>(
                  Result<IntrusivePointer<VectorDocument>>(
                      ErrorList::error(fmt::format(
                          "Error while parsing Cesium ion asset response: "
                          "error {} at byte offset {}.",
                          rapidjson::GetParseError_En(response.GetParseError()),
                          response.GetErrorOffset()))));
        }

        const std::string type =
            JsonHelpers::getStringOrDefault(response, "type", "UNKNOWN");
        if (type != "GEOJSON") {
          return asyncSystem.createResolvedFuture<
              Result<IntrusivePointer<VectorDocument>>>(Result<IntrusivePointer<
                                                            VectorDocument>>(
              ErrorList::error(fmt::format(
                  "Found asset type '{}'. Only GEOJSON is currently supported.",
                  type))));
        }

        std::vector<VectorDocumentAttribution> attributions;

        const auto attributionsMember = response.FindMember("attributions");
        if (attributionsMember != response.MemberEnd() &&
            attributionsMember->value.IsArray()) {
          for (const rapidjson::Value& attribution :
               attributionsMember->value.GetArray()) {
            VectorDocumentAttribution& docAttribution =
                attributions.emplace_back();
            docAttribution.html =
                JsonHelpers::getStringOrDefault(attribution, "html", "");
            docAttribution.showOnScreen = !JsonHelpers::getBoolOrDefault(
                attribution,
                "collapsible",
                true);
          }
        }

        const std::string assetUrl =
            JsonHelpers::getStringOrDefault(response, "url", "");
        const std::string accessToken =
            JsonHelpers::getStringOrDefault(response, "accessToken", "");

        const std::vector<IAssetAccessor::THeader> headers{
            {"Authorization", "Bearer " + accessToken}};
        return pAssetAccessor->get(asyncSystem, assetUrl, headers)
            .thenImmediately(
                [attributions = std::move(attributions)](
                    std::shared_ptr<IAssetRequest>&& pAssetRequest) mutable
                -> Result<IntrusivePointer<VectorDocument>> {
                  const IAssetResponse* pAssetResponse =
                      pAssetRequest->response();

                  if (pAssetResponse->statusCode() < 200 ||
                      pAssetResponse->statusCode() >= 300) {
                    return Result<IntrusivePointer<VectorDocument>>(
                        ErrorList::error(fmt::format(
                            "Status code {} while requesting Cesium ion "
                            "vector asset data.",
                            pAssetResponse->statusCode())));
                  }

                  return VectorDocument::fromGeoJson(
                      pAssetResponse->data(),
                      std::move(attributions));
                });
      });
}

VectorDocument::VectorDocument(
    VectorNode&& rootNode,
    std::vector<VectorDocumentAttribution>&& attributions)
    : _rootNode(std::move(rootNode)), _attributions(std::move(attributions)) {}

const VectorNode& VectorDocument::getRootNode() const { return _rootNode; }
VectorNode& VectorDocument::getRootNode() { return _rootNode; }
} // namespace CesiumVectorData