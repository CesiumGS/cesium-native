#include "Cesium3DTilesSelection/TileContentLoadInput.h"

#include "Cesium3DTilesSelection/Tileset.h"

#include <CesiumAsync/IAssetResponse.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;

TileContentLoadInput::TileContentLoadInput(const Tile& tile)
    : asyncSystem(nullptr),
      pLogger(nullptr),
      pAssetAccessor(nullptr),
      pRequest(nullptr),
      tileID(tile.getTileID()),
      tileBoundingVolume(tile.getBoundingVolume()),
      tileContentBoundingVolume(tile.getContentBoundingVolume()),
      tileRefine(tile.getRefine()),
      tileGeometricError(tile.getGeometricError()),
      tileTransform(tile.getTransform()),
      contentOptions(tile.getContext()->pTileset->getOptions().contentOptions) {
}

TileContentLoadInput::TileContentLoadInput(
    const AsyncSystem& asyncSystem_,
    const std::shared_ptr<spdlog::logger>& pLogger_,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor_,
    const std::shared_ptr<IAssetRequest>& pRequest_,
    const std::shared_ptr<IAssetRequest>& pSubtreeRequest_,
    const Tile& tile)
    : asyncSystem(asyncSystem_),
      pLogger(pLogger_),
      pAssetAccessor(pAssetAccessor_),
      pRequest(pRequest_),
      pSubtreeRequest(pSubtreeRequest_),
      tileID(tile.getTileID()),
      tileBoundingVolume(tile.getBoundingVolume()),
      tileContentBoundingVolume(tile.getContentBoundingVolume()),
      tileRefine(tile.getRefine()),
      tileGeometricError(tile.getGeometricError()),
      tileTransform(tile.getTransform()),
      contentOptions(tile.getContext()->pTileset->getOptions().contentOptions) {
}

TileContentLoadInput::TileContentLoadInput(
    const AsyncSystem& asyncSystem_,
    const std::shared_ptr<spdlog::logger>& pLogger_,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor_,
    const std::shared_ptr<IAssetRequest>& pRequest_,
    const std::shared_ptr<IAssetRequest>& pSubtreeRequest_,
    const TileID& tileID_,
    const BoundingVolume& tileBoundingVolume_,
    const std::optional<BoundingVolume>& tileContentBoundingVolume_,
    TileRefine tileRefine_,
    double tileGeometricError_,
    const glm::dmat4& tileTransform_,
    const TilesetContentOptions& contentOptions_)
    : asyncSystem(asyncSystem_),
      pLogger(pLogger_),
      pAssetAccessor(pAssetAccessor_),
      pRequest(pRequest_),
      pSubtreeRequest(pSubtreeRequest_),
      tileID(tileID_),
      tileBoundingVolume(tileBoundingVolume_),
      tileContentBoundingVolume(tileContentBoundingVolume_),
      tileRefine(tileRefine_),
      tileGeometricError(tileGeometricError_),
      tileTransform(tileTransform_),
      contentOptions(contentOptions_) {}
