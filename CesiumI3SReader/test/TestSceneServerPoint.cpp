#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/GunzipAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumCurl/CurlAssetAccessor.h>
#include <CesiumI3S/Fields.h>
#include <CesiumI3S/Scene.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3S/Store.h>
#include <CesiumI3SReader/SceneLayerReader.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>

#include <doctest/doctest.h>

#include <memory>
#include <string>

// 2015 Street Tree Survey (NYC) -- layerType: Point
// https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services/2015_Street_Tree_Survey_v18/SceneServer/layers/0

TEST_CASE("SceneServer Point layer -- 2015 Street Tree Survey [live]") {
  const std::string url =
      "https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services"
      "/2015_Street_Tree_Survey_v18/SceneServer/layers/0";

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
  auto result = reader.readLayer(pResponse->data());

  REQUIRE(result.errors.empty());
  REQUIRE(result.value.has_value());

  const CesiumI3S::Layer& layer = *result.value;

  CHECK(layer.id == 0);
  REQUIRE(layer.name.has_value());
  CHECK(*layer.name == "NYC_2015_Tree_Census");
  CHECK(layer.layerType == CesiumI3S::SceneLayerType::Point);
  CHECK(layer.version == "272000AB-9FAD-4CF2-887D-36E62AB94CDE");

  // Spatial reference
  REQUIRE(layer.spatialReference.has_value());
  CHECK(layer.spatialReference->wkid == 4326);
  CHECK(layer.spatialReference->latestWkid == 4326);

  // Capabilities: View, Query
  REQUIRE(layer.capabilities.size() == 2);
  CHECK(layer.capabilities[0] == CesiumI3S::LayerCapabilities::View);
  CHECK(layer.capabilities[1] == CesiumI3S::LayerCapabilities::Query);

  // Height model info
  REQUIRE(layer.heightModelInfo.has_value());
  REQUIRE(layer.heightModelInfo->heightModel.has_value());
  CHECK(
      *layer.heightModelInfo->heightModel ==
      CesiumI3S::HeightModel::GravityRelatedHeight);
  CHECK(layer.heightModelInfo->vertCrs == "EGM96_Geoid");
  CHECK(layer.heightModelInfo->heightUnit == CesiumI3S::HeightUnit::Meter);

  // Full extent
  REQUIRE(layer.fullExtent.has_value());
  CHECK(layer.fullExtent->xmin == doctest::Approx(-74.4482627895254));
  CHECK(layer.fullExtent->xmax == doctest::Approx(-73.6024558913962));

  // Store -- profile "points" maps to Profile::Point
  CHECK(layer.store.version == "1.8");
  CHECK(layer.store.profile == CesiumI3S::Store::Profile::Point);
  CHECK(layer.store.rootNode == "./nodes/root");

  // Fields -- 42 total (FID + 41 attribute fields)
  REQUIRE(layer.fields.has_value());
  CHECK(layer.fields->size() == 42);
  CHECK(layer.fields->at(0).name == "FID");
  CHECK(layer.fields->at(0).type == CesiumI3S::Field::Type::OID);
  CHECK(layer.fields->at(1).name == "address");
  CHECK(layer.fields->at(1).type == CesiumI3S::Field::Type::String);

  // Attribute storage -- one entry per field
  REQUIRE(layer.attributeStorageInfo.has_value());
  CHECK(layer.attributeStorageInfo->size() == 42);
  CHECK(layer.attributeStorageInfo->at(0).key == "f_0");
  CHECK(layer.attributeStorageInfo->at(0).name == "FID");

  // Elevation info -- "onTheGround"
  REQUIRE(layer.elevationInfo.has_value());
  CHECK(layer.elevationInfo->mode == CesiumI3S::ElevationMode::OnTheGround);

  // Node pages
  REQUIRE(layer.nodePages.has_value());
  CHECK(layer.nodePages->nodesPerPage == 64);
  CHECK(
      layer.nodePages->lodSelectionMetricType ==
      CesiumI3S::LodSelection::Metric::MaxScreenThresholdSQ);

  // Geometry definitions -- draco-only buffer
  CHECK(layer.geometryDefinitions.size() == 1);
  REQUIRE(layer.geometryDefinitions[0].geometryBuffers.size() == 1);
  REQUIRE(
      layer.geometryDefinitions[0].geometryBuffers[0].compression.has_value());
  CHECK(
      layer.geometryDefinitions[0].geometryBuffers[0].compression->encoding ==
      CesiumI3S::Compression::Encoding::Draco);
}
