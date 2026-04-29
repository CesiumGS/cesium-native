#include "Cesium3DTilesContent/ImplicitTilingUtilities.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumNativeTests/FileAccessor.h"
#include "CesiumNativeTests/SimpleTaskProcessor.h"
#include "CesiumRasterOverlays/ActivatedRasterOverlay.h"
#include "CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h"
#include "CesiumRasterOverlays/RasterOverlayExternals.h"
#include "CesiumRasterOverlays/RasterOverlayTile.h"
#include "CesiumUtility/IntrusivePointer.h"
#include <Cesium3DTilesSelection/VectorTilesRasterOverlay.h>

#include <doctest/doctest.h>

using Cesium3DTilesSelection::VectorTilesRasterOverlay;
using CesiumAsync::AsyncSystem;
using CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters;
using CesiumRasterOverlays::RasterOverlayOptions;
using CesiumUtility::IntrusivePointer;

TEST_CASE("Test VectorTilesRasterOverlay") {
  const glm::dvec2 imageSize(256, 256);
  
  const CesiumGeospatial::BoundingRegion& tileRegion = Cesium3DTilesContent::ImplicitTilingUtilities::computeBoundingVolume(
      CesiumGeospatial::BoundingRegion{
          CesiumGeospatial::GlobeRectangle{-3.141592653589793, -1.5707963267948966, 0.0, 1.5707963267948966},
          0,
          0.005,
          CesiumGeospatial::Ellipsoid::WGS84},
      CesiumGeometry::QuadtreeTileID(10, 582, 752),
      CesiumGeospatial::Ellipsoid::WGS84);
  const CesiumGeospatial::GlobeRectangle& tileGlobeRectangle = tileRegion.getRectangle();

  std::shared_ptr<CesiumNativeTests::FileAccessor> pAssetAccessor = std::make_shared<CesiumNativeTests::FileAccessor>(CesiumNativeTests::FileAccessor{});
  AsyncSystem asyncSystem{
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()};

  CreateRasterOverlayTileProviderParameters parameters{
      {pAssetAccessor, nullptr, asyncSystem}};
  IntrusivePointer<VectorTilesRasterOverlay> pOverlay;
  pOverlay.emplace(
      "overlay0",
      "file:////mnt/d/Dev/vector/data/patterns-newyork-segments/"
      "patterns-newyork-segments_part_1/tileset.json",
      RasterOverlayOptions{});

  IntrusivePointer<CesiumRasterOverlays::ActivatedRasterOverlay> pActivated = pOverlay->activate(
      CesiumRasterOverlays::RasterOverlayExternals{
          .pAssetAccessor = pAssetAccessor,
          .pPrepareRendererResources = nullptr,
          .asyncSystem = asyncSystem,
          .pCreditSystem = nullptr,
          .pLogger = spdlog::default_logger()},
      CesiumGeospatial::Ellipsoid::WGS84);

  pActivated->getReadyEvent().waitInMainThread();

  REQUIRE(pActivated->getTileProvider() != nullptr);
  CesiumGeospatial::GeographicProjection projection(CesiumGeospatial::Ellipsoid::WGS84);
  const CesiumGeometry::Rectangle tileRectangle = projection.project(tileGlobeRectangle);

  IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile> pTile =
      pActivated->getTile(tileRectangle, imageSize);
  pActivated->loadTile(*pTile);
  while (pTile->getState() != CesiumRasterOverlays::RasterOverlayTile::LoadState::Loaded) {
    asyncSystem.dispatchMainThreadTasks();
    pActivated->tick();
  }
  
  REQUIRE(pTile->getImage()->width > 1);
}