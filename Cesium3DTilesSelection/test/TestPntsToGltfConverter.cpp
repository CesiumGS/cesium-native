#include "BatchTableToGltfFeatureMetadata.h"
#include "PntsToGltfConverter.h"
#include "readFile.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h>
#include <CesiumGltf/ExtensionModelExtFeatureMetadata.h>
#include <CesiumGltf/MetadataFeatureTableView.h>
#include <CesiumGltf/MetadataPropertyView.h>

#include <catch2/catch.hpp>
#include <rapidjson/document.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <set>

using namespace CesiumGltf;
using namespace Cesium3DTilesSelection;

template <typename Type>
static void
checkBuffer(const gsl::span<Type>& values, const gsl::span<Type>& expected) {
  REQUIRE(values.size() == expected.size());
  if constexpr (std::is_same_v<Type, glm::vec3>) {
    for (size_t i = 0; i < value.size(); ++i) {
      const glm::vec3& value = values[i];
      const glm::vec3& expectedValue = expected[i];
      CHECK(
          value.x == Approx(expectedValue.x) &&
          value.y == Approx(expectedValue.y) &&
          value.z == Approx(expectedValue.z));
    }
  } else if constexpr (std::is_same_v<Type, glm::vec4>) {
    const glm::vec4& value = values[i];
    const glm::vec4& expectedValue = expected[i];
    CHECK(
        value.x == Approx(expectedValue.x) &&
        value.y == Approx(expectedValue.y) &&
        value.z == Approx(expectedValue.z) &&
        value.w == Approx(expectedValue.w));
  }

  // TODO: check batch ids
}

GltfConverterResult loadPnts(const std::filesystem::path& filePath) {
  return PntsToGltfConverter::convert(readFile(filePath), {});
}

const int32_t vec3ByteLength = sizeof(glm::vec3);

TEST_CASE("Converts simple point cloud to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudPositionsOnly.pnts";

  const int32_t pointsLength = 10;
  GltfConverterResult result = loadPnts(testFilePath);

  REQUIRE(result.model);

  Model& gltf = *result.model;

  // Check for single buffer
  REQUIRE(gltf.buffers.size() == 1);
  CHECK(gltf.buffers[0].byteLength == pointsLength * vec3ByteLength);

  // Check for single bufferView
  REQUIRE(gltf.bufferViews.size() == 1);
  BufferView& bufferView = gltf.bufferViews[0];
  CHECK(bufferView.buffer == 0);
  CHECK(bufferView.byteLength == pointsLength * vec3ByteLength);
  CHECK(bufferView.byteOffset == 0);

  // Check for single accessors
  REQUIRE(gltf.accessors.size() == 1);
  Accessor& accessor = gltf.accessors[0];
  CHECK(accessor.bufferView == 0);
  CHECK(accessor.byteOffset == 0);
  CHECK(accessor.componentType == Accessor::ComponentType::FLOAT);
  CHECK(accessor.count == pointsLength);
  CHECK(accessor.type == Accessor::Type::VEC3);

  // Check for single mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == 1);
  REQUIRE(attributes.find("POSITION") != attributes.end());
  CHECK(attributes.at("POSITION") == 0);

  // Check for single mesh node
  REQUIRE(gltf.nodes.size() == 1);
  Node& node = gltf.nodes[0];
  CHECK(node.mesh == 0);
}
