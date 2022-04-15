#include "Cesium3DTilesSelection/ExternalTilesetContent.h"

#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>

#include <cstddef>

namespace Cesium3DTilesSelection {

CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
ExternalTilesetContent::load(const TileContentLoadInput& input) {
  return input.asyncSystem.createResolvedFuture(load(
      input.pLogger,
      input.tileTransform,
      input.tileRefine,
      input.pRequest->url(),
      input.pRequest->response()->data()));
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

  std::unique_ptr<TileContext> pExternalTileContext =
      std::make_unique<TileContext>();
  pExternalTileContext->baseUrl = url;
  pExternalTileContext->contextInitializerCallback =
      [](const TileContext& parentContext, TileContext& currentContext) {
        currentContext.pTileset = parentContext.pTileset;
        currentContext.requestHeaders = parentContext.requestHeaders;
        currentContext.version = parentContext.version;
        currentContext.failedTileCallback = parentContext.failedTileCallback;
      };

  TileContext* pContext = pExternalTileContext.get();
  pResult->childTiles.value()[0].setContext(pContext);
  pResult->newTileContexts.push_back(std::move(pExternalTileContext));

  Tileset::loadTilesFromJson(
      pResult->childTiles.value()[0],
      pResult->newTileContexts,
      tilesetJson,
      tileTransform,
      tileRefine,
      *pContext,
      pLogger);

  // Special case: if the root tile of an external tileset has no content,
  // treat it as unrenderable rather than rendering a hole in its place.
  // This is arguably contrary to the 3D Tiles spec (see
  // https://github.com/CesiumGS/3d-tiles/issues/609), but it's very
  // convenient when an external tileset conceptually has multiple roots.
  // It also brings us closer to CesiumJS's behavior.
  const auto rootIt = tilesetJson.FindMember("root");
  if (rootIt != tilesetJson.MemberEnd()) {
    const auto contentIt = rootIt->value.FindMember("content");
    if (contentIt == rootIt->value.MemberEnd()) {
      Tile& rootTile = pResult->childTiles.value()[0];

      // A tile is unrenderable if it _does_ have content, but that content has
      // a _nullopt_ model. Yes, this is confusing.
      rootTile.setEmptyContent();
      rootTile.setState(Tile::LoadState::ContentLoaded);
    }
  }

  return pResult;
}

} // namespace Cesium3DTilesSelection
