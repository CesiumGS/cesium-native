#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/GunzipAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumCurl/CurlAssetAccessor.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/Material.h>
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

// Rancho Mesh v18 -- layerType: IntegratedMesh
// https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services/Rancho_Mesh_v18/SceneServer/layers/0

TEST_CASE("SceneServer IntegratedMesh layer -- Rancho Mesh [live]") {
  const std::string url =
      "https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services"
      "/Rancho_Mesh_v18/SceneServer/layers/0";

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
  CHECK(*layer.name == "Rancho_Mesh_v17");
  CHECK(layer.layerType == CesiumI3S::SceneLayerType::IntegratedMesh);

  // Spatial reference
  REQUIRE(layer.spatialReference.has_value());
  CHECK(layer.spatialReference->wkid == 4326);
  CHECK(layer.spatialReference->latestWkid == 4326);

  // Store
  CHECK(layer.store.version == "1.8");
  CHECK(layer.store.rootNode == "./nodes/root");
  CHECK(
      layer.store.normalReferenceFrame ==
      CesiumI3S::Store::NormalReferenceFrame::EastNorthUp);
  REQUIRE(layer.store.lodType.has_value());
  CHECK(*layer.store.lodType == CesiumI3S::Store::LodType::MeshPyramid);
  REQUIRE(layer.store.lodModel.has_value());
  CHECK(*layer.store.lodModel == CesiumI3S::Store::LodModel::NodeSwitching);
  CHECK(layer.store.profile == CesiumI3S::Store::Profile::MeshPyramid);

  // Height model info: "orthometric"
  REQUIRE(layer.heightModelInfo.has_value());
  REQUIRE(layer.heightModelInfo->heightModel.has_value());
  CHECK(
      *layer.heightModelInfo->heightModel ==
      CesiumI3S::HeightModel::Orthometric);
  CHECK(layer.heightModelInfo->vertCrs == "WGS_84");
  CHECK(layer.heightModelInfo->heightUnit == CesiumI3S::HeightUnit::Meter);

  // Capabilities: View, Query
  REQUIRE(layer.capabilities.size() == 2);
  CHECK(layer.capabilities[0] == CesiumI3S::LayerCapabilities::View);
  CHECK(layer.capabilities[1] == CesiumI3S::LayerCapabilities::Query);

  // Full extent
  REQUIRE(layer.fullExtent.has_value());
  CHECK(layer.fullExtent->xmin == doctest::Approx(-117.53833934397));
  CHECK(layer.fullExtent->xmax == doctest::Approx(-117.53685255909));

  // Material: doubleSided=true, pbrMetallicRoughness present
  REQUIRE(layer.materialDefinitions.size() == 1);
  CHECK(layer.materialDefinitions[0].doubleSided == true);
  REQUIRE(layer.materialDefinitions[0].pbrMetallicRoughness.has_value());
  CHECK(
      layer.materialDefinitions[0].pbrMetallicRoughness->metallicFactor ==
      doctest::Approx(0.0));

  // Texture set definitions: formats jpg + dds
  REQUIRE(layer.textureSetDefinitions.size() == 1);
  REQUIRE(layer.textureSetDefinitions[0].formats.size() == 2);
  CHECK(
      layer.textureSetDefinitions[0].formats[0].format ==
      CesiumI3S::TextureFormat::Jpg);
  CHECK(
      layer.textureSetDefinitions[0].formats[1].format ==
      CesiumI3S::TextureFormat::Dds);

  // Node pages
  REQUIRE(layer.nodePages.has_value());
  CHECK(layer.nodePages->nodesPerPage == 64);
  CHECK(
      layer.nodePages->lodSelectionMetricType ==
      CesiumI3S::LodSelection::Metric::MaxScreenThresholdSQ);

  // Materials and textures
  CHECK(layer.materialDefinitions.size() == 1);
  CHECK(layer.textureSetDefinitions.size() == 1);

  // Geometry definitions -- two buffers: uncompressed + draco
  REQUIRE(layer.geometryDefinitions.size() == 1);
  REQUIRE(layer.geometryDefinitions[0].geometryBuffers.size() == 2);

  // First buffer: uncompressed with offset=8, position/normal/uv0/color
  const auto& rawBuf = layer.geometryDefinitions[0].geometryBuffers[0];
  CHECK(rawBuf.offset == 8);
  CHECK(rawBuf.position.isPresent());
  CHECK(rawBuf.normal.isPresent());
  CHECK(rawBuf.uv0.isPresent());
  CHECK(rawBuf.color.isPresent());

  // Second buffer: draco-compressed
  const auto& dracoBuf = layer.geometryDefinitions[0].geometryBuffers[1];
  REQUIRE(dracoBuf.compression.has_value());
  CHECK(
      dracoBuf.compression->encoding ==
      CesiumI3S::Compression::Encoding::Draco);
}
