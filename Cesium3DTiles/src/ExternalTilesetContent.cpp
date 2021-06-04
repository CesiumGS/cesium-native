#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumUtility/Uri.h"
#include <cstddef>
#include <rapidjson/document.h>

namespace Cesium3DTiles {

CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
ExternalTilesetContent::load(const CesiumAsync::AsyncSystem& asyncSystem, const TileContentLoadInput& input) {
  return asyncSystem.createResolvedFuture(
    load(
      input.pLogger,
      input.tileTransform,
      input.tileRefine,
      input.url,
      input.data));
}

/*static*/ std::unique_ptr<TileContentLoadResult> ExternalTilesetContent::load(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const glm::dmat4& tileTransform,
    TileRefine tileRefine,
    const std::string& url,
    const gsl::span<const std::byte>& data) {
  std::unique_ptr<TileContentLoadResult> pResult =
      std::make_unique<TileContentLoadResult>();

  rapidjson::Document tilesetJson;
  tilesetJson.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (tilesetJson.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing tileset JSON, error code {} at byte offset {}",
        tilesetJson.GetParseError(),
        tilesetJson.GetErrorOffset());
    return pResult;
  }

  pResult->childTiles.emplace(1);

  pResult->pNewTileContext = std::make_unique<TileContext>();
  pResult->pNewTileContext->baseUrl = url;

  TileContext* pContext = pResult->pNewTileContext.get();
  pContext->contextInitializerCallback = [](const TileContext& parentContext,
                                            TileContext& currentContext) {
    currentContext.pTileset = parentContext.pTileset;
    currentContext.requestHeaders = parentContext.requestHeaders;
    currentContext.version = parentContext.version;
    currentContext.failedTileCallback = parentContext.failedTileCallback;
  };

  pResult->childTiles.value()[0].setContext(pContext);

  Tileset::loadTilesFromJson(
      pResult->childTiles.value()[0],
      tilesetJson,
      tileTransform,
      tileRefine,
      *pContext,
      pLogger);

  return pResult;
}

} // namespace Cesium3DTiles
