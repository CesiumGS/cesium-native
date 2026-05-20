#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/GltfConverter.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>

#include <filesystem>

using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;
using namespace CesiumVectorData;

TEST_CASE("Conversion from geoJSON to glTF") {
  SUBCASE("Feature ids") {
    std::filesystem::path dir(
        std::filesystem::path(CesiumVectorData_TEST_DATA_DIR) / "geojson");
    Result<GeoJsonDocument> doc =
        GeoJsonDocument::fromGeoJson(readFile(dir / "ship-trajectories.json"));
    REQUIRE(doc.value);
    ConverterResult converterResult =
        GltfConverter::convert(*doc.value, Ellipsoid::WGS84);
    REQUIRE(converterResult.value);
    const Model& model = *converterResult.value;
    model.forEachPrimitiveInScene(
        -1,
        [](const Model& model,
           const Node&,
           const Mesh&,
           const MeshPrimitive& prim,
           const glm::dmat4&) {
          const ExtensionExtMeshFeatures* pPrimitiveExtension =
              prim.getExtension<ExtensionExtMeshFeatures>();
          REQUIRE(pPrimitiveExtension);
          REQUIRE(!pPrimitiveExtension->featureIds.empty());
          const FeatureId& featureId = pPrimitiveExtension->featureIds[0];
          REQUIRE(featureId.attribute);
          int64_t attribute = *featureId.attribute;
          int32_t positionAccessor = prim.attributes.at("POSITION");
          int64_t positionCount =
              model.accessors[uint32_t(positionAccessor)].count;
          // It would be better to use CesiumGltf::getFeatureIdAccessorView,
          // but that is strict about not allowing UNSIGNED_INT accessors for
          // feature IDs, which the converter returns. Need to decide which
          // way to go on this.
          const std::string attributeName =
              "_FEATURE_ID_" + std::to_string(attribute);
          int32_t featureAccessor = prim.attributes.at(attributeName);
          createAccessorView(
              model,
              featureAccessor,
              [positionCount](auto&& view) {
                CHECK(view.status() == AccessorViewStatus::Valid);
                CHECK(view.size() == positionCount);
              });
        });
  }
}
