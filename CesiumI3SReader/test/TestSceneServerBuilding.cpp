#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/GunzipAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumCurl/CurlAssetAccessor.h>
#include <CesiumI3S/Building.h>
#include <CesiumI3S/Layer.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3SReader/SceneLayerReader.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>

#include <doctest/doctest.h>

#include <memory>
#include <string>

// Admin Building v18 -- layerType: Building (BLD)
// https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services/Admin_Building_v18/SceneServer/layers/0

TEST_CASE("SceneServer Building layer -- Admin Building [live]") {
  const std::string url =
      "https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services"
      "/Admin_Building_v18/SceneServer/layers/0";

  CesiumAsync::AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::ThreadTaskProcessor>());
  CesiumAsync::GunzipAssetAccessor accessor(
      std::make_shared<CesiumCurl::CurlAssetAccessor>(
          CesiumCurl::CurlAssetAccessorOptions{}));

  auto pRequest = accessor.get(asyncSystem, url, {}).waitInMainThread();
  REQUIRE(pRequest);
  const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
  REQUIRE(pResponse);
  REQUIRE(pResponse->statusCode() >= 200);
  REQUIRE(pResponse->statusCode() < 300);

  CesiumI3SReader::SceneLayerReader reader;
  auto result = reader.readBuildingLayer(pResponse->data());

  REQUIRE(result.errors.empty());
  REQUIRE(result.value.has_value());

  const CesiumI3S::BuildingLayer& layer = *result.value;

  CHECK(layer.id == 0);
  CHECK(layer.name == "Admin_Building_v17");
  CHECK(layer.version == "1.8");

  // Spatial reference
  CHECK(layer.spatialReference.wkid == 4326);
  CHECK(layer.spatialReference.latestWkid == 4326);

  // Statistics href
  REQUIRE(layer.statisticsHRef.has_value());
  CHECK(*layer.statisticsHRef == "./statistics/summary");

  // Top-level sublayers: "Full Model" (id=200) and "Overview" (id=0)
  REQUIRE(layer.sublayers.size() == 2);
  CHECK(layer.sublayers[0].id == 200);
  CHECK(layer.sublayers[0].name == "Full Model");
  CHECK(layer.sublayers[0].layerType == CesiumI3S::Sublayer::Type::Group);
  CHECK(layer.sublayers[1].id == 0);
  CHECK(layer.sublayers[1].name == "Overview");
  CHECK(
      layer.sublayers[1].layerType == CesiumI3S::Sublayer::Type::ThreeDObject);

  // Full Model has 4 nested discipline groups (Piping, Electrical, Structural,
  // Architectural)
  REQUIRE(layer.sublayers[0].sublayers.has_value());
  CHECK(layer.sublayers[0].sublayers->size() == 4);

  // This electrical group contains 7 3DObject sublayers
  const CesiumI3S::Sublayer& electrical = layer.sublayers[0].sublayers->at(1);
  CHECK(electrical.name == "Electrical");
  CHECK(electrical.layerType == CesiumI3S::Sublayer::Type::Group);
  REQUIRE(electrical.sublayers.has_value());
  CHECK(electrical.sublayers->size() == 7);

  // Full extent
  CHECK(layer.fullExtent.xmin == doctest::Approx(-117.18710192525656));
  CHECK(layer.fullExtent.xmax == doctest::Approx(-117.18653016431237));
}
