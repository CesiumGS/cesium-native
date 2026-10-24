#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/GunzipAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumCurl/CurlAssetAccessor.h>
#include <CesiumI3S/Layer.h>
#include <CesiumI3S/PointCloud.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3SReader/SceneLayerReader.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>

#include <doctest/doctest.h>

#include <memory>
#include <string>

// Moro Bay LiDAR -- layerType: PointCloud (I3S 2.0 PCSL)
// https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services/Moro_Bay_LiDAR/SceneServer/layers/0

TEST_CASE("SceneServer PointCloud layer -- Moro Bay LiDAR [live]") {
  const std::string url =
      "https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services"
      "/Moro_Bay_LiDAR/SceneServer/layers/0";

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
  auto result = reader.readPointCloudLayer(pResponse->data());

  REQUIRE(result.errors.empty());
  REQUIRE(result.value.has_value());

  const CesiumI3S::PointCloudLayer& layer = *result.value;

  CHECK(layer.id == 0);
  CHECK(layer.name == "Moro_Bay_LiDAR");

  // Capabilities: View, Extract
  REQUIRE(layer.capabilities.size() == 2);
  CHECK(layer.capabilities[0] == CesiumI3S::LayerCapabilities::View);
  CHECK(layer.capabilities[1] == CesiumI3S::LayerCapabilities::Extract);

  // Spatial reference
  CHECK(layer.spatialReference.wkid == 4326);
  CHECK(layer.spatialReference.latestWkid == 4326);

  // Height model info: gravity-related, EGM96_Geoid, meter
  REQUIRE(layer.heightModelInfo.has_value());
  REQUIRE(layer.heightModelInfo->heightModel.has_value());
  CHECK(
      *layer.heightModelInfo->heightModel ==
      CesiumI3S::HeightModel::GravityRelatedHeight);
  CHECK(layer.heightModelInfo->vertCrs == "EGM96_Geoid");
  CHECK(layer.heightModelInfo->heightUnit == CesiumI3S::HeightUnit::Meter);

  // Elevation info: absoluteHeight
  REQUIRE(layer.elevationInfo.has_value());
  CHECK(layer.elevationInfo->mode == CesiumI3S::ElevationMode::AbsoluteHeight);

  // Attribute storage: 6 LiDAR channels
  CHECK(layer.attributeStorageInfo.size() == 6);
  CHECK(layer.attributeStorageInfo.at(0).name == "ELEVATION");
  CHECK(layer.attributeStorageInfo.at(1).name == "INTENSITY");
  CHECK(layer.attributeStorageInfo.at(2).name == "RGB");

  // Store version
  CHECK(layer.store.version == "2.0");

  // Index
  CHECK(layer.store.index.nodeVersion == 1);
  CHECK(layer.store.index.nodesPerPage == 64);
  CHECK(
      layer.store.index.boundingVolumeType ==
      CesiumI3S::BoundingVolume::OrientedBoundingBox);
  CHECK(
      layer.store.index.lodSelectionMetricType ==
      CesiumI3S::LodSelection::Metric::DensityThreshold);

  // Default geometry schema
  CHECK(
      layer.store.defaultGeometrySchema.geometryType ==
      CesiumI3S::GeometryTopology::Point);
  CHECK(
      layer.store.defaultGeometrySchema.topology ==
      CesiumI3S::GeometrySchema::BufferLayout::PerAttributeArray);
  CHECK(
      layer.store.defaultGeometrySchema.encoding ==
      CesiumI3S::Compression::Encoding::Lepcc);

  // Position vertex attribute -- 3 values per point (X, Y, Z)
  REQUIRE(layer.store.defaultGeometrySchema.vertexAttributes.position
              .valuesPerElement.has_value());
  CHECK(
      *layer.store.defaultGeometrySchema.vertexAttributes.position
           .valuesPerElement == 3u);
}
