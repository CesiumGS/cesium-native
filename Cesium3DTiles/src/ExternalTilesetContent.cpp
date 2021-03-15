#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumUtility/Uri.h"
#include <cstddef>
#include <rapidjson/document.h>

namespace Cesium3DTiles {

std::unique_ptr<TileContentLoadResult>
ExternalTilesetContent::load(const TileContentLoadInput& input) {
  return load(
      input.pLogger,
      input.tileTransform,
      input.tileRefine,
      input.url,
      input.data);
}

/*static*/ std::unique_ptr<TileContentLoadResult> ExternalTilesetContent::load(
    std::shared_ptr<spdlog::logger> pLogger,
    const glm::dmat4& /* tileTransform */,
    TileRefine /* tileRefine */,
    const std::string& /* url */,
    const gsl::span<const std::byte>& /* data */) {
  return nullptr;
  // std::unique_ptr<TileContentLoadResult> pResult =
  //     std::make_unique<TileContentLoadResult>();

  // rapidjson::Document tilesetJson;
  // tilesetJson.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  // if (tilesetJson.HasParseError()) {
  //   SPDLOG_LOGGER_ERROR(
  //       pLogger,
  //       "Error when parsing tileset JSON, error code {} at byte offset {}",
  //       tilesetJson.GetParseError(),
  //       tilesetJson.GetErrorOffset());
  //   return pResult;
  // }

  // pResult->childTiles.emplace(1);

  // pResult->pNewTileContext = std::make_unique<TileContext>();
  // TileContext* pContext = pResult->pNewTileContext.get();
  // pContext->pTileset = context.pTileset;
  // pContext->baseUrl = url;
  // pContext->requestHeaders = context.requestHeaders;
  // pContext->version = context.version;
  // pContext->failedTileCallback = context.failedTileCallback;

  // pResult->childTiles.value()[0].setContext(pContext);

  // context.pTileset->loadTilesFromJson(
  //     pResult->childTiles.value()[0],
  //     tilesetJson,
  //     tileTransform,
  //     tileRefine,
  //     *pContext,
  //     pLogger);

  // return pResult;
}

} // namespace Cesium3DTiles
