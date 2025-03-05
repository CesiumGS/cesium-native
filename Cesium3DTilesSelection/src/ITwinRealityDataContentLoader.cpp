#include "ITwinRealityDataContentLoader.h"

#include "CesiumAsync/Future.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumUtility/JsonHelpers.h"
#include "CesiumUtility/Uri.h"
#include "ITwinUtilities.h"
#include "TilesetJsonLoader.h"

namespace Cesium3DTilesSelection {

namespace {
enum class RealityDataType {
  Cesium3DTiles,
  RealityMesh3DTiles,
  Terrain3DTiles,
  Pnts,
  Unsupported
};

struct RealityDataDetails {
  std::string id;
  std::string rootDocument;
  RealityDataType type;
};

std::optional<RealityDataDetails>
parseRealityDataDetails(const rapidjson::Document& jsonDocument) {
  const auto& realityData = jsonDocument.FindMember("realityData");
  if (realityData == jsonDocument.MemberEnd() ||
      !realityData->value.IsObject()) {
    return std::nullopt;
  }

  const rapidjson::Value::ConstObject& realityDataObj =
      realityData->value.GetObject();

  RealityDataDetails details;
  details.id =
      CesiumUtility::JsonHelpers::getStringOrDefault(realityDataObj, "id", "");
  details.rootDocument = CesiumUtility::JsonHelpers::getStringOrDefault(
      realityDataObj,
      "rootDocument",
      "");

  const std::string typeStr = CesiumUtility::JsonHelpers::getStringOrDefault(
      realityDataObj,
      "type",
      "");
  // Types from
  // https://developer.bentley.com/apis/reality-management/rm-rd-details/#types
  if (typeStr == "Cesium3DTiles") {
    details.type = RealityDataType::Cesium3DTiles;
  } else if (typeStr == "PNTS") {
    details.type = RealityDataType::Pnts;
  } else if (typeStr == "RealityMesh3DTiles") {
    details.type = RealityDataType::RealityMesh3DTiles;
  } else if (typeStr == "Terrain3DTiles") {
    details.type = RealityDataType::Terrain3DTiles;
  } else {
    details.type = RealityDataType::Unsupported;
  }

  return details;
}

std::string getContainerUrl(const rapidjson::Document& jsonDocument) {
  const auto& links = jsonDocument.FindMember("_links");
  if (links == jsonDocument.MemberEnd() || !links->value.IsObject()) {
    return {};
  }

  const auto& containerUrl =
      links->value.GetObject().FindMember("containerUrl");
  if (containerUrl == jsonDocument.MemberEnd() ||
      !containerUrl->value.IsObject()) {
    return {};
  }

  return CesiumUtility::JsonHelpers::getStringOrDefault(
      containerUrl->value,
      "href",
      "");
}

CesiumAsync::Future<TilesetContentLoaderResult<ITwinRealityDataContentLoader>>
requestRealityDataContainer(
    const TilesetExternals& externals,
    const RealityDataDetails& details,
    const std::optional<std::string>& iTwinId,
    const std::string& iTwinAccessToken,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CesiumUtility::Uri readAccessUri(fmt::format(
      "https://api.bentley.com/reality-management/reality-data/{}/readaccess",
      details.id));
  if (iTwinId.has_value()) {
    CesiumUtility::UriQuery query("");
    query.setValue("iTwinId", *iTwinId);
    readAccessUri.setQuery(query.toQueryString());
  }

  const std::vector<CesiumAsync::IAssetAccessor::THeader> headers = {
      {"Authorization", "Bearer " + iTwinAccessToken},
      {"Accept", "application/vnd.bentley.itwin-platform.v1+json"}};

  return externals.pAssetAccessor
      ->get(
          externals.asyncSystem,
          std::string{readAccessUri.toString()},
          headers)
      .thenImmediately([externals,
                        details,
                        iTwinId,
                        iTwinAccessToken,
                        ellipsoid](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                       pRequest) {
        const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
        const std::string& requestUrl = pRequest->url();
        if (!pResponse) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "No response received for reality data read access request {}",
              requestUrl));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode < 200 || statusCode >= 300) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for reality data read access response "
              "{}",
              statusCode,
              requestUrl));
          result.statusCode = statusCode;
          parseITwinErrorResponseIntoErrorList(pResponse, result.errors);
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        const std::span<const std::byte> data = pResponse->data();

        rapidjson::Document readAccessResponse;
        readAccessResponse.Parse(
            reinterpret_cast<const char*>(data.data()),
            data.size());

        const std::string parsedContainerUrl =
            getContainerUrl(readAccessResponse);
        if (parsedContainerUrl.empty()) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "Couldn't obtain container URL for reality data {}",
              details.id));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        // containerUrl provides the directory that contains the data,
        // rootDocument provides the file that contains the tileset info, we
        // need to combine the two.
        CesiumUtility::Uri containerUri(parsedContainerUrl);
        containerUri.setPath(
            fmt::format("{}/{}", containerUri.getPath(), details.rootDocument));

        std::vector<CesiumAsync::IAssetAccessor::THeader> tilesetHeaders{
            {"Authorization", "Bearer " + iTwinAccessToken}};

        return TilesetJsonLoader::createLoader(
                   externals,
                   std::string{containerUri.toString()},
                   tilesetHeaders,
                   ellipsoid)
            .thenImmediately([tilesetHeaders](
                                 TilesetContentLoaderResult<TilesetJsonLoader>&&
                                     tilesetJsonResult) mutable {
              TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
              if (!tilesetJsonResult.errors) {
                result.pLoader =
                    std::make_unique<ITwinRealityDataContentLoader>(
                        std::move(tilesetJsonResult.pLoader));
                result.pRootTile = std::move(tilesetJsonResult.pRootTile);
                result.credits = std::move(tilesetJsonResult.credits);
                result.requestHeaders = std::move(tilesetHeaders);
              }
              result.errors = std::move(tilesetJsonResult.errors);
              result.statusCode = tilesetJsonResult.statusCode;
              return result;
            });
      });
}

} // namespace

CesiumAsync::Future<TilesetContentLoaderResult<ITwinRealityDataContentLoader>>
ITwinRealityDataContentLoader::createLoader(
    const TilesetExternals& externals,
    const std::string& realityDataId,
    const std::optional<std::string>& iTwinId,
    const std::string& iTwinAccessToken,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CesiumUtility::Uri realityMetadataUri(
      "https://api.bentley.com/reality-management/reality-data/" +
      realityDataId);
  if (iTwinId.has_value()) {
    CesiumUtility::UriQuery query("");
    query.setValue("iTwinId", *iTwinId);
    realityMetadataUri.setQuery(query.toQueryString());
  }

  const std::vector<CesiumAsync::IAssetAccessor::THeader> headers = {
      {"Authorization", "Bearer " + iTwinAccessToken},
      {"Accept", "application/vnd.bentley.itwin-platform.v1+json"}};

  return externals.pAssetAccessor
      ->get(
          externals.asyncSystem,
          std::string{realityMetadataUri.toString()},
          headers)
      .thenImmediately([externals,
                        realityDataId,
                        iTwinId,
                        iTwinAccessToken,
                        ellipsoid](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                       pRequest) {
        const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
        const std::string& requestUrl = pRequest->url();
        if (!pResponse) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "No response received for reality data metadata request {}",
              requestUrl));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode < 200 || statusCode >= 300) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for reality data metadata response {}",
              statusCode,
              requestUrl));
          result.statusCode = statusCode;
          parseITwinErrorResponseIntoErrorList(pResponse, result.errors);
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        const std::span<const std::byte> data = pResponse->data();

        rapidjson::Document metadataResponse;
        metadataResponse.Parse(
            reinterpret_cast<const char*>(data.data()),
            data.size());

        const std::optional<RealityDataDetails> details =
            parseRealityDataDetails(metadataResponse);
        if (!details || details->type == RealityDataType::Unsupported) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "No 3D Tiles reality data found for id {}",
              realityDataId));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        return requestRealityDataContainer(
            externals,
            *details,
            iTwinId,
            iTwinAccessToken,
            ellipsoid);
      });
}
CesiumAsync::Future<TileLoadResult>
ITwinRealityDataContentLoader::loadTileContent(const TileLoadInput& loadInput) {
  return this->_pAggregatedLoader->loadTileContent(loadInput);
}
TileChildrenResult ITwinRealityDataContentLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  auto pLoader = tile.getLoader();
  return pLoader->createTileChildren(tile, ellipsoid);
}
} // namespace Cesium3DTilesSelection