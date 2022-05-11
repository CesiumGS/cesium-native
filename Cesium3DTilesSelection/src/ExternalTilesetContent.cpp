#include "Cesium3DTilesSelection/ExternalTilesetContent.h"

#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>

#include <cstddef>

namespace Cesium3DTilesSelection {
//
// CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
// ExternalTilesetContent::load(const TileContentLoadInput& input) {
//  return input.asyncSystem.createResolvedFuture(load(
//      input.pLogger,
//      input.tileTransform,
//      input.tileRefine,
//      input.pRequest->url(),
//      input.pRequest->response()->data()));
//}
//
///*static*/ std::unique_ptr<TileContentLoadResult>
/// ExternalTilesetContent::load(
//    const std::shared_ptr<spdlog::logger>& pLogger,
//    const glm::dmat4& tileTransform,
//    TileRefine tileRefine,
//    const std::string& url,
//    const gsl::span<const std::byte>& data) {
//  std::unique_ptr<TileContentLoadResult> pResult =
//      std::make_unique<TileContentLoadResult>();
//
//  rapidjson::Document tilesetJson;
//  tilesetJson.Parse(reinterpret_cast<const char*>(data.data()), data.size());
//
//  if (tilesetJson.HasParseError()) {
//    SPDLOG_LOGGER_ERROR(
//        pLogger,
//        "Error when parsing tileset JSON, error code {} at byte offset {}",
//        tilesetJson.GetParseError(),
//        tilesetJson.GetErrorOffset());
//    return pResult;
//  }
//
//  pResult->childTiles.emplace(1);
//
//  std::unique_ptr<TileContext> pExternalTileContext =
//      std::make_unique<TileContext>();
//  pExternalTileContext->baseUrl = url;
//  pExternalTileContext->contextInitializerCallback =
//      [](const TileContext& parentContext, TileContext& currentContext) {
//        currentContext.pTileset = parentContext.pTileset;
//        currentContext.requestHeaders = parentContext.requestHeaders;
//        currentContext.version = parentContext.version;
//        currentContext.failedTileCallback = parentContext.failedTileCallback;
//      };
//
//  TileContext* pContext = pExternalTileContext.get();
//  pResult->childTiles.value()[0].setContext(pContext);
//  pResult->newTileContexts.push_back(std::move(pExternalTileContext));
//
//  Tileset::loadTilesFromJson(
//      pResult->childTiles.value()[0],
//      pResult->newTileContexts,
//      tilesetJson,
//      tileTransform,
//      tileRefine,
//      *pContext,
//      pLogger);
//
//  return pResult;
//}

} // namespace Cesium3DTilesSelection
