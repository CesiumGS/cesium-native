#include "TerrainLayerJson.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

CesiumAsync::Future<TerrainLayerJson> TerrainLayerJson::resolveParents(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    const std::vector<IAssetAccessor::THeader>& requestHeaders) && {
  if (this->parentUrl.empty()) {
    return asyncSystem.createResolvedFuture<TerrainLayerJson>(std::move(*this));
  }

  std::string resolvedUrl = Uri::resolve(baseUrl, this->parentUrl);
  return pAssetAccessor->requestAsset(asyncSystem, resolvedUrl, requestHeaders)
      .thenInWorkerThread(
          [layerJson = std::move(*this),
           asyncSystem,
           pAssetAccessor,
           pLogger,
           requestHeaders](std::shared_ptr<IAssetRequest>&& pRequest) mutable {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Did not receive a valid response for parent layer.json {}",
                  pRequest->url());
              return asyncSystem.createResolvedFuture<TerrainLayerJson>(
                  std::move(layerJson));
            }

            if (pResponse->statusCode() != 0 &&
                (pResponse->statusCode() < 200 ||
                 pResponse->statusCode() >= 300)) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Received status code {} for parent layer.json {}",
                  pResponse->statusCode(),
                  pRequest->url());
              return asyncSystem.createResolvedFuture<TerrainLayerJson>(
                  std::move(layerJson));
            }

            TerrainLayerJson parent =
                TerrainLayerJson::parse(pLogger, pResponse->data());

            // Also resolve this parent layer.json's parentUrl, if any.
            return std::move(parent)
                .resolveParents(
                    asyncSystem,
                    pAssetAccessor,
                    pLogger,
                    pRequest->url(),
                    requestHeaders)
                .thenImmediately([layerJson = std::move(layerJson)](
                                     TerrainLayerJson&& parent) mutable {
                  layerJson.pResolvedParent =
                      std::make_unique<TerrainLayerJson>(std::move(parent));
                  return std::move(layerJson);
                });
          });
}

/*static*/ TerrainLayerJson TerrainLayerJson::parse(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const gsl::span<const std::byte>& data) {
  rapidjson::Document document;
  document.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (document.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing layer.json, error code {} at byte offset "
        "{}",
        document.GetParseError(),
        document.GetErrorOffset());
    return TerrainLayerJson();
  }

  return TerrainLayerJson::parse(pLogger, document);
}

/*static*/ TerrainLayerJson TerrainLayerJson::parse(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const rapidjson::Value& jsonValue) {
  TerrainLayerJson result;

  if (!jsonValue.IsObject()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Could not parse terrain layer.json because it is not a JSON object.");
    return result;
  }

  result.attribution =
      JsonHelpers::getStringOrDefault(jsonValue, "attribution", "");
  result.description =
      JsonHelpers::getStringOrDefault(jsonValue, "description", "");
  result.extensions = JsonHelpers::getStrings(jsonValue, "extensions");
  result.format = JsonHelpers::getStringOrDefault(jsonValue, "format", "");
  result.maxzoom = JsonHelpers::getInt32OrDefault(jsonValue, "maxzoom", 30);
  result.metadataAvailability =
      JsonHelpers::getInt32OrDefault(jsonValue, "metadataAvailability", 0);
  result.minzoom = JsonHelpers::getInt32OrDefault(jsonValue, "minzoom", 30);
  result.parentUrl =
      JsonHelpers::getStringOrDefault(jsonValue, "parentUrl", "");
  result.projection =
      JsonHelpers::getStringOrDefault(jsonValue, "projection", "");
  result.scheme = JsonHelpers::getStringOrDefault(jsonValue, "scheme", "");
  result.tiles = JsonHelpers::getStrings(jsonValue, "tiles");
  result.version = JsonHelpers::getStringOrDefault(jsonValue, "version", "");

  std::optional<std::vector<double>> bounds =
      JsonHelpers::getDoubles(jsonValue, 4, "bounds");
  if (bounds) {
    result.bounds = GlobeRectangle::fromDegrees(
        bounds->at(0),
        bounds->at(1),
        bounds->at(2),
        bounds->at(3));
  }

  // If there's a metadata extension we'll get availability from there, so don't
  // waste time parsing the `available` property.
  if (std::find(
          result.extensions.begin(),
          result.extensions.end(),
          "metadata") == result.extensions.end()) {
    // TODO: parse availability
  }

  return result;
}
