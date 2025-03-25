#include "IModelMeshExportContentLoader.h"

#include "ITwinUtilities.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace Cesium3DTilesSelection {

namespace {
struct IModelMeshExport {
  std::string id;
  std::string meshHref;
};

std::vector<IModelMeshExport>
parseGetExportsResponse(const rapidjson::Document& response) {
  std::vector<IModelMeshExport> exports;
  const auto exportsIt = response.FindMember("exports");
  if (exportsIt != response.MemberEnd() && exportsIt->value.IsArray()) {
    for (const rapidjson::Value& parsedExport : exportsIt->value.GetArray()) {
      const std::string& exportId =
          CesiumUtility::JsonHelpers::getStringOrDefault(
              parsedExport,
              "id",
              "");

      // Obtain _links.mesh.href if possible
      std::string meshHref;
      const auto links = parsedExport.FindMember("_links");
      if (links != parsedExport.MemberEnd() && links->value.IsObject()) {
        const auto meshLink = links->value.FindMember("mesh");
        if (meshLink != links->value.MemberEnd() &&
            meshLink->value.IsObject()) {
          meshHref = CesiumUtility::JsonHelpers::getStringOrDefault(
              meshLink->value,
              "href",
              "");
        }
      }

      // We only want to return exports that have the required values
      if (!exportId.empty() && !meshHref.empty()) {
        exports.emplace_back(IModelMeshExport{exportId, meshHref});
      }
    }
  }

  return exports;
}

} // namespace

CesiumAsync::Future<TilesetContentLoaderResult<IModelMeshExportContentLoader>>
IModelMeshExportContentLoader::createLoader(
    const TilesetExternals& externals,
    const std::string& iModelId,
    const std::optional<std::string>& exportId,
    const std::string& iTwinAccessToken,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CesiumUtility::Uri getExportsUri("https://api.bentley.com/mesh-export/");
  CesiumUtility::UriQuery getExportsQueryParams;
  getExportsQueryParams.setValue("iModelId", iModelId);
  getExportsQueryParams.setValue("exportType", "3DTiles");
  getExportsQueryParams.setValue("$orderBy", "date:desc");
  getExportsUri.setQuery(getExportsQueryParams.toQueryString());

  std::vector<CesiumAsync::IAssetAccessor::THeader> headers{
      {"Authorization", "Bearer " + iTwinAccessToken},
      {"Prefer", "return=representation"},
      {"Accept", "application/vnd.bentley.itwin-platform.v1+json"}};

  return externals.pAssetAccessor
      ->get(
          externals.asyncSystem,
          std::string(getExportsUri.toString()),
          headers)
      .thenImmediately([externals,
                        iModelId,
                        exportId,
                        iTwinAccessToken,
                        ellipsoid](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                       pRequest) {
        const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
        const std::string& requestUrl = pRequest->url();
        if (!pResponse) {
          TilesetContentLoaderResult<IModelMeshExportContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "No response received for asset request {}",
              requestUrl));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode < 200 || statusCode >= 300) {
          TilesetContentLoaderResult<IModelMeshExportContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for asset response {}",
              statusCode,
              requestUrl));
          result.statusCode = statusCode;
          parseITwinErrorResponseIntoErrorList(*pResponse, result.errors);
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        const std::span<const std::byte> data = pResponse->data();

        rapidjson::Document iModelResponse;
        iModelResponse.Parse(
            reinterpret_cast<const char*>(data.data()),
            data.size());

        if (iModelResponse.HasParseError()) {
          TilesetContentLoaderResult<IModelMeshExportContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "Error when parsing iModel Mesh Export service response JSON, "
              "error code {} at byte "
              "offset {}",
              iModelResponse.GetParseError(),
              iModelResponse.GetErrorOffset()));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        std::vector<IModelMeshExport> exports =
            parseGetExportsResponse(iModelResponse);
        if (exports.empty()) {
          TilesetContentLoaderResult<IModelMeshExportContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "No 3D Tiles exports found for iModel ID {}",
              iModelId));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        TilesetContentLoaderResult<IModelMeshExportContentLoader> result;
        IModelMeshExport& exportToUse = exports.front();
        // Attempt to find the specified export ID if one was set.
        if (exportId.has_value()) {
          const auto foundExportIt = std::find_if(
              exports.begin(),
              exports.end(),
              [exportId](const IModelMeshExport& meshExport) {
                return meshExport.id == *exportId;
              });

          if (foundExportIt != exports.end()) {
            exportToUse = *foundExportIt;
          } else {
            result.errors.emplaceWarning(fmt::format(
                "No export ID {} found on iModel {}, using most recently "
                "modified export",
                *exportId,
                iModelId));
          }
        }

        // Mesh Export service returns the root directory of the tileset - we
        // need to manually append "/tileset.json"
        CesiumUtility::Uri meshUri(exportToUse.meshHref);
        std::string meshPath{meshUri.getPath()};
        meshUri.setPath(meshPath + "/tileset.json");

        std::vector<CesiumAsync::IAssetAccessor::THeader> tilesetHeaders{
            {"Authorization", "Bearer " + iTwinAccessToken}};

        return TilesetJsonLoader::createLoader(
                   externals,
                   std::string{meshUri.toString()},
                   tilesetHeaders,
                   ellipsoid)
            .thenImmediately([tilesetHeaders](
                                 TilesetContentLoaderResult<TilesetJsonLoader>&&
                                     tilesetJsonResult) mutable {
              TilesetContentLoaderResult<IModelMeshExportContentLoader> result;
              if (!tilesetJsonResult.errors) {
                result.pLoader =
                    std::make_unique<IModelMeshExportContentLoader>(
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

CesiumAsync::Future<TileLoadResult>
IModelMeshExportContentLoader::loadTileContent(const TileLoadInput& loadInput) {
  return this->_pAggregatedLoader->loadTileContent(loadInput);
}

TileChildrenResult IModelMeshExportContentLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  auto pLoader = tile.getLoader();
  return pLoader->createTileChildren(tile, ellipsoid);
}

IModelMeshExportContentLoader::IModelMeshExportContentLoader(
    std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader)
    : _pAggregatedLoader(std::move(pAggregatedLoader)) {}
} // namespace Cesium3DTilesSelection