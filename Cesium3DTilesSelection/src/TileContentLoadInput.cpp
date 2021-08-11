#include "Cesium3DTilesSelection/TileContentLoadInput.h"
#include "Cesium3DTilesSelection/Tileset.h"

using namespace Cesium3DTilesSelection;

TileContentLoadInput::TileContentLoadInput(
    const std::shared_ptr<spdlog::logger> pLogger_,
    const Tile& tile_)
    : pLogger(pLogger_),
      data(),
      contentType(),
      url(),
      tileID(tile_.getTileID()),
      tileBoundingVolume(tile_.getBoundingVolume()),
      tileContentBoundingVolume(tile_.getContentBoundingVolume()),
      tileRefine(tile_.getRefine()),
      tileGeometricError(tile_.getGeometricError()),
      tileTransform(tile_.getTransform()),
      contentOptions(
          tile_.getContext()->pTileset->getOptions().contentOptions) {}

TileContentLoadInput::TileContentLoadInput(
    const std::shared_ptr<spdlog::logger> pLogger_,
    const gsl::span<const std::byte>& data_,
    const std::string& contentType_,
    const std::string& url_,
    const Tile& tile_)
    : pLogger(pLogger_),
      data(data_),
      contentType(contentType_),
      url(url_),
      tileID(tile_.getTileID()),
      tileBoundingVolume(tile_.getBoundingVolume()),
      tileContentBoundingVolume(tile_.getContentBoundingVolume()),
      tileRefine(tile_.getRefine()),
      tileGeometricError(tile_.getGeometricError()),
      tileTransform(tile_.getTransform()),
      contentOptions(
          tile_.getContext()->pTileset->getOptions().contentOptions) {}

TileContentLoadInput::TileContentLoadInput(
    const std::shared_ptr<spdlog::logger> pLogger_,
    const gsl::span<const std::byte>& data_,
    const std::string& contentType_,
    const std::string& url_,
    const TileID& tileID_,
    const BoundingVolume& tileBoundingVolume_,
    const std::optional<BoundingVolume>& tileContentBoundingVolume_,
    TileRefine tileRefine_,
    double tileGeometricError_,
    const glm::dmat4& tileTransform_,
    const TilesetContentOptions& contentOptions_)
    : pLogger(pLogger_),
      data(data_),
      contentType(contentType_),
      url(url_),
      tileID(tileID_),
      tileBoundingVolume(tileBoundingVolume_),
      tileContentBoundingVolume(tileContentBoundingVolume_),
      tileRefine(tileRefine_),
      tileGeometricError(tileGeometricError_),
      tileTransform(tileTransform_),
      contentOptions(contentOptions_) {}
