#include "BatchTableToGltfFeatureMetadata.h"
#include "PntsToGltfConverter.h"
#include "readFile.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
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
static void checkBufferContents(
    const std::vector<std::byte>& buffer,
    const std::vector<Type>& expected) {
  REQUIRE(buffer.size() == expected.size() * sizeof(Type));
  const int32_t byteStride = sizeof(Type);
  if constexpr (std::is_same_v<Type, glm::vec3>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const glm::vec3& value =
          *reinterpret_cast<const glm::vec3*>(buffer.data() + i * byteStride);
      const glm::vec3& expectedValue = expected[i];
      CHECK(value.x == Approx(expectedValue.x));
      CHECK(value.y == Approx(expectedValue.y));
      CHECK(value.z == Approx(expectedValue.z));
    }
  } else if constexpr (std::is_same_v<Type, glm::vec4>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const glm::vec4& value =
          *reinterpret_cast<const glm::vec4*>(buffer.data() + i * byteStride);
      const glm::vec4& expectedValue = expected[i];
      CHECK(value.x == Approx(expectedValue.x));
      CHECK(value.y == Approx(expectedValue.y));
      CHECK(value.z == Approx(expectedValue.z));
      CHECK(value.w == Approx(expectedValue.w));
    }
  } else if constexpr (
      std::is_same_v<Type, glm::u8vec3> || std::is_same_v<Type, glm::u8vec4>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const Type& value =
          *reinterpret_cast<const Type*>(buffer.data() + i * byteStride);
      const Type& expectedValue = expected[i];
      CHECK(value == expectedValue);
    }
  } else {
    FAIL("Buffer check has not been implemented for the given type.");
  }
}

template <typename Type>
static void checkAttribute(
    const Model& gltf,
    const MeshPrimitive& primitive,
    const std::string& attributeSemantic,
    const uint32_t expectedCount) {
  const auto& attributes = primitive.attributes;
  REQUIRE(attributes.find(attributeSemantic) != attributes.end());

  const int32_t accessorId = attributes.at(attributeSemantic);
  REQUIRE(accessorId >= 0);
  REQUIRE(accessorId < gltf.accessors.size());
  const Accessor& accessor = gltf.accessors[accessorId];

  int32_t expectedComponentType = -1;
  std::string expectedType;

  if constexpr (std::is_same_v<Type, glm::vec3>) {
    expectedComponentType = Accessor::ComponentType::FLOAT;
    expectedType = Accessor::Type::VEC3;
  } else {
    FAIL("Accessor check has not been implemented for the given type.");
  }

  CHECK(accessor.byteOffset == 0);
  CHECK(accessor.componentType == expectedComponentType);
  CHECK(accessor.count == expectedCount);
  CHECK(accessor.type == expectedType);

  const int32_t expectedByteLength = expectedCount * sizeof(Type);

  const int32_t bufferViewId = accessor.bufferView;
  REQUIRE(bufferViewId >= 0);
  REQUIRE(bufferViewId < gltf.bufferViews.size());
  const BufferView& bufferView = gltf.bufferViews[bufferViewId];
  CHECK(bufferView.byteLength == expectedByteLength);
  CHECK(bufferView.byteOffset == 0);

  const int32_t bufferId = bufferView.buffer;
  REQUIRE(bufferId >= 0);
  REQUIRE(bufferId < gltf.buffers.size());

  const Buffer& buffer = gltf.buffers[bufferId];
  CHECK(buffer.byteLength == expectedByteLength);
  CHECK(static_cast<int64_t>(buffer.cesium.data.size()) == buffer.byteLength);
}

GltfConverterResult loadPnts(const std::filesystem::path& filePath) {
  return PntsToGltfConverter::convert(readFile(filePath), {});
}

TEST_CASE("Converts simple point cloud to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudPositionsOnly.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = loadPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  // Check for single mesh node
  REQUIRE(gltf.nodes.size() == 1);
  Node& node = gltf.nodes[0];
  CHECK(node.mesh == 0);

  // Check for single mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.material == 0);

  // Check for single material
  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.pbrMetallicRoughness);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == 1);
  REQUIRE(attributes.find("POSITION") != attributes.end());
  CHECK(attributes.at("POSITION") == 0);

  // Check for single accessor
  REQUIRE(gltf.accessors.size() == 1);
  Accessor& accessor = gltf.accessors[0];
  CHECK(accessor.bufferView == 0);
  CHECK(accessor.byteOffset == 0);
  CHECK(accessor.componentType == Accessor::ComponentType::FLOAT);
  CHECK(accessor.count == pointsLength);
  CHECK(accessor.type == Accessor::Type::VEC3);

  const glm::vec3 expectedMin(
      -3.2968313694000244,
      -4.033046722412109,
      -3.522307872772217);
  CHECK(accessor.min[0] == Approx(expectedMin.x));
  CHECK(accessor.min[1] == Approx(expectedMin.y));
  CHECK(accessor.min[2] == Approx(expectedMin.z));

  const glm::vec3 expectedMax(
      3.2968313694000244,
      4.033046722412109,
      3.522307872772217);
  CHECK(accessor.max[0] == Approx(expectedMax.x));
  CHECK(accessor.max[1] == Approx(expectedMax.y));
  CHECK(accessor.max[2] == Approx(expectedMax.z));

  // Check for single bufferView
  REQUIRE(gltf.bufferViews.size() == 1);
  BufferView& bufferView = gltf.bufferViews[0];
  CHECK(bufferView.buffer == 0);
  CHECK(bufferView.byteLength == pointsLength * sizeof(glm::vec3));
  CHECK(bufferView.byteOffset == 0);

  // Check for single buffer
  REQUIRE(gltf.buffers.size() == 1);
  Buffer& buffer = gltf.buffers[0];
  CHECK(buffer.byteLength == pointsLength * sizeof(glm::vec3));
  CHECK(static_cast<int64_t>(buffer.cesium.data.size()) == buffer.byteLength);

  const std::vector<glm::vec3> expectedPositions = {
      glm::vec3(-2.4975082, -0.3252686, -3.5223078),
      glm::vec3(2.3456699, 0.9171584, -3.5223078),
      glm::vec3(-3.2968313, 2.7906193, 0.3055275),
      glm::vec3(1.5463469, 4.03304672, 0.3055275),
      glm::vec3(-1.5463469, -4.03304672, -0.3055275),
      glm::vec3(3.2968313, -2.7906193, -0.3055275),
      glm::vec3(-2.3456699, -0.9171584, 3.5223078),
      glm::vec3(2.4975082, 0.3252686, 3.5223078)};

  checkBufferContents<glm::vec3>(buffer.cesium.data, expectedPositions);

  // Check for RTC extension
  REQUIRE(gltf.hasExtension<CesiumGltf::ExtensionCesiumRTC>());
  const auto& rtcExtension =
      result.model->getExtension<CesiumGltf::ExtensionCesiumRTC>();
  const glm::vec3 expectedRtcCenter(
      1215012.8828876,
      -4736313.0511995,
      4081605.2212604);
  CHECK(rtcExtension->center[0] == Approx(expectedRtcCenter.x));
  CHECK(rtcExtension->center[1] == Approx(expectedRtcCenter.y));
  CHECK(rtcExtension->center[2] == Approx(expectedRtcCenter.z));
}

TEST_CASE("Converts point cloud with RGBA to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudRGBA.pnts";
  const int32_t pointsLength = 8;
  const int32_t expectedAttributeCount = 2;

  GltfConverterResult result = loadPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  CHECK(gltf.hasExtension<CesiumGltf::ExtensionCesiumRTC>());
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.alphaMode == Material::AlphaMode::BLEND);

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position attribute is present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);

  // Check color attribute more thoroughly
  REQUIRE(attributes.find("COLOR_0") != attributes.end());
  int32_t colorAccessorId = attributes.at("COLOR_0");
  REQUIRE(colorAccessorId >= 0);
  REQUIRE(colorAccessorId < expectedAttributeCount);

  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(colorAccessor.byteOffset == 0);
  CHECK(colorAccessor.componentType == Accessor::ComponentType::UNSIGNED_BYTE);
  CHECK(colorAccessor.count == pointsLength);
  CHECK(colorAccessor.type == Accessor::Type::VEC4);
  CHECK(colorAccessor.normalized);

  int32_t colorBufferViewId = colorAccessor.bufferView;
  REQUIRE(colorBufferViewId >= 0);
  REQUIRE(colorBufferViewId < expectedAttributeCount);

  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];
  CHECK(colorBufferView.byteLength == pointsLength * sizeof(glm::u8vec4));
  CHECK(colorBufferView.byteOffset == 0);

  int32_t colorBufferId = colorBufferView.buffer;
  REQUIRE(colorBufferId >= 0);
  REQUIRE(colorBufferId < expectedAttributeCount);

  Buffer& colorBuffer = gltf.buffers[colorBufferId];
  CHECK(colorBuffer.byteLength == pointsLength * sizeof(glm::u8vec4));
  CHECK(
      static_cast<int64_t>(colorBuffer.cesium.data.size()) ==
      colorBuffer.byteLength);

  const std::vector<glm::u8vec4> expectedColors = {
      glm::u8vec4(139, 151, 182, 108),
      glm::u8vec4(153, 218, 138, 108),
      glm::u8vec4(108, 159, 164, 49),
      glm::u8vec4(111, 75, 227, 7),
      glm::u8vec4(245, 69, 97, 61),
      glm::u8vec4(201, 207, 134, 61),
      glm::u8vec4(144, 100, 236, 107),
      glm::u8vec4(18, 86, 22, 82)};

  checkBufferContents<glm::u8vec4>(colorBuffer.cesium.data, expectedColors);
}

TEST_CASE("Converts point cloud with RGB to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudRGB.pnts";
  const int32_t pointsLength = 8;
  const int32_t expectedAttributeCount = 2;

  GltfConverterResult result = loadPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  CHECK(gltf.hasExtension<CesiumGltf::ExtensionCesiumRTC>());
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.alphaMode == Material::AlphaMode::OPAQUE);

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position attribute is present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);

  // Check color attribute more thoroughly
  REQUIRE(attributes.find("COLOR_0") != attributes.end());
  int32_t colorAccessorId = attributes.at("COLOR_0");
  REQUIRE(colorAccessorId >= 0);
  REQUIRE(colorAccessorId < expectedAttributeCount);

  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(colorAccessor.byteOffset == 0);
  CHECK(colorAccessor.componentType == Accessor::ComponentType::UNSIGNED_BYTE);
  CHECK(colorAccessor.count == pointsLength);
  CHECK(colorAccessor.type == Accessor::Type::VEC3);
  CHECK(colorAccessor.normalized);

  int32_t colorBufferViewId = colorAccessor.bufferView;
  REQUIRE(colorBufferViewId >= 0);
  REQUIRE(colorBufferViewId < expectedAttributeCount);

  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];
  CHECK(colorBufferView.byteLength == pointsLength * sizeof(glm::u8vec3));
  CHECK(colorBufferView.byteOffset == 0);

  int32_t colorBufferId = colorBufferView.buffer;
  REQUIRE(colorBufferId >= 0);
  REQUIRE(colorBufferId < expectedAttributeCount);

  Buffer& colorBuffer = gltf.buffers[colorBufferId];
  CHECK(colorBuffer.byteLength == pointsLength * sizeof(glm::u8vec3));
  CHECK(
      static_cast<int64_t>(colorBuffer.cesium.data.size()) ==
      colorBuffer.byteLength);

  const std::vector<glm::u8vec3> expectedColors = {
      glm::u8vec3(139, 151, 182),
      glm::u8vec3(153, 218, 138),
      glm::u8vec3(108, 159, 164),
      glm::u8vec3(111, 75, 227),
      glm::u8vec3(245, 69, 97),
      glm::u8vec3(201, 207, 134),
      glm::u8vec3(144, 100, 236),
      glm::u8vec3(18, 86, 22)};

  checkBufferContents<glm::u8vec3>(colorBuffer.cesium.data, expectedColors);
}

TEST_CASE("Converts point cloud with RGB565 to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudRGB565.pnts";
  const int32_t pointsLength = 8;
  const int32_t expectedAttributeCount = 2;

  GltfConverterResult result = loadPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  CHECK(gltf.hasExtension<CesiumGltf::ExtensionCesiumRTC>());
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.alphaMode == Material::AlphaMode::OPAQUE);

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position attribute is present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);

  // Check color attribute more thoroughly
  REQUIRE(attributes.find("COLOR_0") != attributes.end());
  int32_t colorAccessorId = attributes.at("COLOR_0");
  REQUIRE(colorAccessorId >= 0);
  REQUIRE(colorAccessorId < expectedAttributeCount);

  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(colorAccessor.byteOffset == 0);
  CHECK(colorAccessor.componentType == Accessor::ComponentType::FLOAT);
  CHECK(colorAccessor.count == pointsLength);
  CHECK(colorAccessor.type == Accessor::Type::VEC3);
  CHECK(!colorAccessor.normalized);

  int32_t colorBufferViewId = colorAccessor.bufferView;
  REQUIRE(colorBufferViewId >= 0);
  REQUIRE(colorBufferViewId < expectedAttributeCount);

  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];
  CHECK(colorBufferView.byteLength == pointsLength * sizeof(glm::vec3));
  CHECK(colorBufferView.byteOffset == 0);

  int32_t colorBufferId = colorBufferView.buffer;
  REQUIRE(colorBufferId >= 0);
  REQUIRE(colorBufferId < expectedAttributeCount);

  Buffer& colorBuffer = gltf.buffers[colorBufferId];
  CHECK(colorBuffer.byteLength == pointsLength * sizeof(glm::vec3));
  CHECK(
      static_cast<int64_t>(colorBuffer.cesium.data.size()) ==
      colorBuffer.byteLength);

  const std::vector<glm::vec3> expectedColors = {
      glm::vec3(0.5483871, 0.5873016, 0.7096773),
      glm::vec3(0.5806451, 0.8571428, 0.5161290),
      glm::vec3(0.4193548, 0.6190476, 0.6451612),
      glm::vec3(0.4193548, 0.2857142, 0.8709677),
      glm::vec3(0.9354838, 0.2698412, 0.3548386),
      glm::vec3(0.7741935, 0.8095238, 0.5161290),
      glm::vec3(0.5483871, 0.3809523, 0.9032257),
      glm::vec3(0.0645161, 0.3333333, 0.0645161)};

  checkBufferContents<glm::vec3>(colorBuffer.cesium.data, expectedColors);
}

TEST_CASE("Converts point cloud with CONSTANT_RGBA") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudConstantRGBA.pnts";
  GltfConverterResult result = loadPnts(testFilePath);
  const int32_t pointsLength = 8;

  REQUIRE(result.model);
  Model& gltf = *result.model;

  CHECK(gltf.hasExtension<CesiumGltf::ExtensionCesiumRTC>());
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.material == 0);

  CHECK(gltf.buffers.size() == 1);
  CHECK(gltf.bufferViews.size() == 1);
  CHECK(gltf.accessors.size() == 1);

  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  REQUIRE(material.pbrMetallicRoughness);
  MaterialPBRMetallicRoughness& pbrMetallicRoughness =
      material.pbrMetallicRoughness.value();
  const auto& baseColorFactor = pbrMetallicRoughness.baseColorFactor;

  // Check that CONSTANT_RGBA is stored in the material base color
  const glm::vec4 expectedConstantRGBA =
      glm::vec4(1.0f, 1.0f, 0.0f, 51.0f / 255.0f);
  CHECK(baseColorFactor[0] == Approx(expectedConstantRGBA.x));
  CHECK(baseColorFactor[1] == Approx(expectedConstantRGBA.y));
  CHECK(baseColorFactor[2] == Approx(expectedConstantRGBA.z));
  CHECK(baseColorFactor[3] == Approx(expectedConstantRGBA.w));

  CHECK(material.alphaMode == Material::AlphaMode::BLEND);
}
