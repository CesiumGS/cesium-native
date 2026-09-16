#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/GunzipAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumCurl/CurlAssetAccessor.h>
#include <CesiumI3S/Fields.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/Node.h>
#include <CesiumI3S/Scene.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3S/Store.h>
#include <CesiumI3S/TextureDefinition.h>
#include <CesiumI3SReader/SceneLayerReader.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>

#include <doctest/doctest.h>

#include <memory>
#include <string>

// Montreal Buildings v18 -- layerType: 3DObject
// https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services/Montreal_Buildings_v18/SceneServer/layers/0

TEST_CASE("SceneServer 3DObject layer -- Montreal Buildings [live]") {
  const std::string url =
      "https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services"
      "/Montreal_Buildings_v18/SceneServer/layers/0";

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
  CHECK(*layer.name == "Montreal_Buildings_v17_21778");
  CHECK(layer.layerType == CesiumI3S::SceneLayerType::ThreeDObject);
  CHECK(layer.version == "25DA815A-206E-4C5A-B0C5-AAE108C47120");

  // Spatial reference
  REQUIRE(layer.spatialReference.has_value());
  CHECK(layer.spatialReference->wkid == 4326);
  CHECK(layer.spatialReference->latestWkid == 4326);

  // Store
  CHECK(layer.store.version == "1.8");
  CHECK(layer.store.rootNode == "./nodes/root");
  CHECK(
      layer.store.normalReferenceFrame ==
      CesiumI3S::Store::NormalReferenceFrame::EarthCentered);
  REQUIRE(layer.store.lodType.has_value());
  CHECK(*layer.store.lodType == CesiumI3S::Store::LodType::MeshPyramid);
  REQUIRE(layer.store.lodModel.has_value());
  CHECK(*layer.store.lodModel == CesiumI3S::Store::LodModel::NodeSwitching);
  REQUIRE(layer.store.resourcePattern.has_value());
  REQUIRE(layer.store.resourcePattern->size() == 4);
  CHECK(
      layer.store.resourcePattern->at(0) ==
      CesiumI3S::Store::ResourcePattern::NodeIndexDocument);
  CHECK(
      layer.store.resourcePattern->at(1) ==
      CesiumI3S::Store::ResourcePattern::SharedResource);
  CHECK(
      layer.store.resourcePattern->at(2) ==
      CesiumI3S::Store::ResourcePattern::Geometry);
  CHECK(
      layer.store.resourcePattern->at(3) ==
      CesiumI3S::Store::ResourcePattern::Attributes);

  // Height model info
  REQUIRE(layer.heightModelInfo.has_value());
  REQUIRE(layer.heightModelInfo->heightModel.has_value());
  CHECK(
      *layer.heightModelInfo->heightModel ==
      CesiumI3S::HeightModel::GravityRelatedHeight);
  CHECK(layer.heightModelInfo->vertCrs == "EGM96_Geoid");
  CHECK(layer.heightModelInfo->heightUnit == CesiumI3S::HeightUnit::Meter);

  // Capabilities: View, Query
  REQUIRE(layer.capabilities.size() == 2);
  CHECK(layer.capabilities[0] == CesiumI3S::LayerCapabilities::View);
  CHECK(layer.capabilities[1] == CesiumI3S::LayerCapabilities::Query);

  // Full extent
  REQUIRE(layer.fullExtent.has_value());
  CHECK(layer.fullExtent->xmin == doctest::Approx(-73.6276007755874));
  CHECK(layer.fullExtent->xmax == doctest::Approx(-73.5106170136111));
  CHECK(layer.fullExtent->ymin == doctest::Approx(45.472628213605));
  CHECK(layer.fullExtent->ymax == doctest::Approx(45.5467391543877));

  // Texture set definitions: first has atlas=true, formats jpg + dds
  REQUIRE(layer.textureSetDefinitions[0].formats.size() == 2);
  CHECK(
      layer.textureSetDefinitions[0].formats[0].format ==
      CesiumI3S::TextureFormat::Jpg);
  CHECK(
      layer.textureSetDefinitions[0].formats[1].format ==
      CesiumI3S::TextureFormat::Dds);
  CHECK(layer.textureSetDefinitions[0].atlas == true);

  // Fields: OBJECTID + 7 attribute fields = 8 total
  REQUIRE(layer.fields.has_value());
  CHECK(layer.fields->size() == 8);
  CHECK(layer.fields->at(0).name == "OBJECTID");
  CHECK(layer.fields->at(0).type == CesiumI3S::Field::Type::OID);
  CHECK(layer.fields->at(1).name == "BuildingShellFID");
  CHECK(layer.fields->at(1).type == CesiumI3S::Field::Type::String);

  // Attribute storage info
  REQUIRE(layer.attributeStorageInfo.has_value());
  CHECK(layer.attributeStorageInfo->size() == 8);
  CHECK(layer.attributeStorageInfo->at(0).key == "f_0");
  CHECK(layer.attributeStorageInfo->at(0).name == "OBJECTID");

  // Node pages
  REQUIRE(layer.nodePages.has_value());
  CHECK(layer.nodePages->nodesPerPage == 64);
  CHECK(
      layer.nodePages->lodSelectionMetricType ==
      CesiumI3S::LodSelection::Metric::MaxScreenThresholdSQ);

  // Materials: 2 PBR definitions
  CHECK(layer.materialDefinitions.size() == 2);

  // Texture set definitions: 2
  CHECK(layer.textureSetDefinitions.size() == 2);

  // Geometry definitions: 2 (with uvRegion and without)
  REQUIRE(layer.geometryDefinitions.size() == 2);

  // First geometry def: raw buffer (with uvRegion) + draco
  REQUIRE(layer.geometryDefinitions[0].geometryBuffers.size() == 2);
  const auto& rawBuf0 = layer.geometryDefinitions[0].geometryBuffers[0];
  CHECK(rawBuf0.offset == 8);
  CHECK(rawBuf0.position.isPresent());
  CHECK(rawBuf0.normal.isPresent());
  CHECK(rawBuf0.uv0.isPresent());
  CHECK(rawBuf0.uvRegion.isPresent());

  // Second geometry def: raw buffer (no uvRegion) + draco
  REQUIRE(layer.geometryDefinitions[1].geometryBuffers.size() == 2);
  const auto& rawBuf1 = layer.geometryDefinitions[1].geometryBuffers[0];
  CHECK(rawBuf1.offset == 8);
  CHECK_FALSE(rawBuf1.uvRegion.isPresent());

  // Both second buffers are draco-compressed
  REQUIRE(
      layer.geometryDefinitions[0].geometryBuffers[1].compression.has_value());
  REQUIRE(
      layer.geometryDefinitions[1].geometryBuffers[1].compression.has_value());
}
