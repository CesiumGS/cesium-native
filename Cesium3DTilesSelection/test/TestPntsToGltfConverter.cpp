#include "BatchTableToGltfFeatureMetadata.h"
#include "ConvertTileToGltf.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/ExtensionKhrMaterialsUnlit.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h>
#include <CesiumGltf/ExtensionModelExtFeatureMetadata.h>
#include <CesiumGltf/MetadataFeatureTableView.h>
#include <CesiumGltf/MetadataPropertyView.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>
#include <rapidjson/document.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <set>

using namespace CesiumGltf;
using namespace Cesium3DTilesSelection;
using namespace CesiumUtility;

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
      CHECK(Math::equalsEpsilon(
          static_cast<glm::dvec3>(value),
          static_cast<glm::dvec3>(expectedValue),
          Math::Epsilon6));
    }
  } else if constexpr (std::is_same_v<Type, glm::vec4>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const glm::vec4& value =
          *reinterpret_cast<const glm::vec4*>(buffer.data() + i * byteStride);
      const glm::vec4& expectedValue = expected[i];
      CHECK(Math::equalsEpsilon(
          static_cast<glm::dvec4>(value),
          static_cast<glm::dvec4>(expectedValue),
          Math::Epsilon6));
    }
  } else if constexpr (std::is_floating_point_v<Type>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const Type& value =
          *reinterpret_cast<const Type*>(buffer.data() + i * byteStride);
      const Type& expectedValue = expected[i];
      CHECK(value == Approx(expectedValue));
    }
  } else if constexpr (
      std::is_integral_v<Type> || std::is_same_v<Type, glm::u8vec2> ||
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
  const uint32_t accessorIdUint = static_cast<uint32_t>(accessorId);
  const Accessor& accessor = gltf.accessors[accessorIdUint];

  int32_t expectedComponentType = -1;
  std::string expectedType;

  if constexpr (std::is_same_v<Type, glm::vec3>) {
    expectedComponentType = Accessor::ComponentType::FLOAT;
    expectedType = Accessor::Type::VEC3;
  } else if constexpr (std::is_same_v<Type, glm::u8vec3>) {
    expectedComponentType = Accessor::ComponentType::UNSIGNED_BYTE;
    expectedType = Accessor::Type::VEC3;
  } else if constexpr (std::is_same_v<Type, glm::u8vec4>) {
    expectedComponentType = Accessor::ComponentType::UNSIGNED_BYTE;
    expectedType = Accessor::Type::VEC4;
  } else if constexpr (std::is_same_v<Type, uint8_t>) {
    expectedComponentType = Accessor::ComponentType::UNSIGNED_BYTE;
    expectedType = Accessor::Type::SCALAR;
  } else {
    FAIL("Accessor check has not been implemented for the given type.");
  }

  CHECK(accessor.byteOffset == 0);
  CHECK(accessor.componentType == expectedComponentType);
  CHECK(accessor.count == expectedCount);
  CHECK(accessor.type == expectedType);

  const int64_t expectedByteLength =
      static_cast<int64_t>(expectedCount * sizeof(Type));

  const int32_t bufferViewId = accessor.bufferView;
  REQUIRE(bufferViewId >= 0);
  const uint32_t bufferViewIdUint = static_cast<uint32_t>(bufferViewId);
  const BufferView& bufferView = gltf.bufferViews[bufferViewIdUint];
  CHECK(bufferView.byteLength == expectedByteLength);
  CHECK(bufferView.byteOffset == 0);

  const int32_t bufferId = bufferView.buffer;
  REQUIRE(bufferId >= 0);
  const uint32_t bufferIdUint = static_cast<uint32_t>(bufferId);
  const Buffer& buffer = gltf.buffers[static_cast<uint32_t>(bufferIdUint)];
  CHECK(buffer.byteLength == expectedByteLength);
  CHECK(static_cast<int64_t>(buffer.cesium.data.size()) == buffer.byteLength);
}

TEST_CASE("Converts simple point cloud to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudPositionsOnly.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  // Check for single mesh node
  REQUIRE(gltf.nodes.size() == 1);
  Node& node = gltf.nodes[0];
  // clang-format off
  std::vector<double> expectedMatrix = {
      1.0, 0.0, 0.0, 0.0,
      0.0, 0.0, -1.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 1.0};
  // clang-format on
  CHECK(node.matrix == expectedMatrix);
  CHECK(node.mesh == 0);

  // Check for single mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);
  CHECK(primitive.material == 0);

  // Check for single material
  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.pbrMetallicRoughness);
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());

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

  const glm::vec3 expectedMin(-3.2968313, -4.0330467, -3.5223078);
  CHECK(accessor.min[0] == Approx(expectedMin.x));
  CHECK(accessor.min[1] == Approx(expectedMin.y));
  CHECK(accessor.min[2] == Approx(expectedMin.z));

  const glm::vec3 expectedMax(3.2968313, 4.0330467, 3.5223078);
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

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

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
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position and color attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::u8vec4>(gltf, primitive, "COLOR_0", pointsLength);

  // Check color attribute more thoroughly
  uint32_t colorAccessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(colorAccessor.normalized);

  uint32_t colorBufferViewId = static_cast<uint32_t>(colorAccessor.bufferView);
  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];

  uint32_t colorBufferId = static_cast<uint32_t>(colorBufferView.buffer);
  Buffer& colorBuffer = gltf.buffers[colorBufferId];

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

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

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
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position and color attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::u8vec3>(gltf, primitive, "COLOR_0", pointsLength);

  // Check color attribute more thoroughly
  uint32_t colorAccessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(colorAccessor.normalized);

  uint32_t colorBufferViewId = static_cast<uint32_t>(colorAccessor.bufferView);
  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];

  uint32_t colorBufferId = static_cast<uint32_t>(colorBufferView.buffer);
  Buffer& colorBuffer = gltf.buffers[colorBufferId];

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

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

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
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position and color attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);

  // Check color attribute more thoroughly
  // Check color attribute more thoroughly
  uint32_t colorAccessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(!colorAccessor.normalized);

  uint32_t colorBufferViewId = static_cast<uint32_t>(colorAccessor.bufferView);
  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];

  uint32_t colorBufferId = static_cast<uint32_t>(colorBufferView.buffer);
  Buffer& colorBuffer = gltf.buffers[colorBufferId];

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
  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);
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
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());
}

TEST_CASE("Converts point cloud with quantized positions to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudQuantized.pnts";
  const int32_t pointsLength = 8;
  const int32_t expectedAttributeCount = 2;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  CHECK(!gltf.hasExtension<CesiumGltf::ExtensionCesiumRTC>());
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position and color attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::u8vec3>(gltf, primitive, "COLOR_0", pointsLength);

  // Check position attribute more thoroughly
  uint32_t positionAccessorId =
      static_cast<uint32_t>(attributes.at("POSITION"));
  Accessor& positionAccessor = gltf.accessors[positionAccessorId];
  CHECK(!positionAccessor.normalized);

  const glm::vec3 expectedMin(1215009.59, -4736317.08, 4081601.7);
  CHECK(positionAccessor.min[0] == Approx(expectedMin.x));
  CHECK(positionAccessor.min[1] == Approx(expectedMin.y));
  CHECK(positionAccessor.min[2] == Approx(expectedMin.z));

  const glm::vec3 expectedMax(1215016.18, -4736309.02, 4081608.74);
  CHECK(positionAccessor.max[0] == Approx(expectedMax.x));
  CHECK(positionAccessor.max[1] == Approx(expectedMax.y));
  CHECK(positionAccessor.max[2] == Approx(expectedMax.z));

  uint32_t positionBufferViewId =
      static_cast<uint32_t>(positionAccessor.bufferView);
  BufferView& positionBufferView = gltf.bufferViews[positionBufferViewId];

  uint32_t positionBufferId = static_cast<uint32_t>(positionBufferView.buffer);
  Buffer& positionBuffer = gltf.buffers[positionBufferId];

  const std::vector<glm::vec3> expectedPositions = {
      glm::vec3(1215010.39, -4736313.38, 4081601.7),
      glm::vec3(1215015.23, -4736312.13, 4081601.7),
      glm::vec3(1215009.59, -4736310.26, 4081605.53),
      glm::vec3(1215014.43, -4736309.02, 4081605.53),
      glm::vec3(1215011.34, -4736317.08, 4081604.92),
      glm::vec3(1215016.18, -4736315.84, 4081604.92),
      glm::vec3(1215010.54, -4736313.97, 4081608.74),
      glm::vec3(1215015.38, -4736312.73, 4081608.74)};

  checkBufferContents<glm::vec3>(positionBuffer.cesium.data, expectedPositions);
}

TEST_CASE("Converts point cloud with normals to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudNormals.pnts";
  const int32_t pointsLength = 8;
  const int32_t expectedAttributeCount = 3;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

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
  CHECK(!material.hasExtension<ExtensionKhrMaterialsUnlit>());

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position, color, and normal attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::u8vec3>(gltf, primitive, "COLOR_0", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "NORMAL", pointsLength);

  // Check normal attribute more thoroughly
  uint32_t normalAccessorId = static_cast<uint32_t>(attributes.at("NORMAL"));
  Accessor& normalAccessor = gltf.accessors[normalAccessorId];

  uint32_t normalBufferViewId =
      static_cast<uint32_t>(normalAccessor.bufferView);
  BufferView& normalBufferView = gltf.bufferViews[normalBufferViewId];

  uint32_t normalBufferId = static_cast<uint32_t>(normalBufferView.buffer);
  Buffer& normalBuffer = gltf.buffers[normalBufferId];

  const std::vector<glm::vec3> expectedNormals = {
      glm::vec3(-0.9854088, 0.1667507, 0.0341110),
      glm::vec3(-0.5957704, 0.5378777, 0.5964436),
      glm::vec3(-0.5666092, -0.7828890, -0.2569800),
      glm::vec3(-0.5804154, -0.7226123, 0.3754320),
      glm::vec3(-0.8535281, -0.1291752, -0.5047805),
      glm::vec3(0.7557975, 0.1243999, 0.6428800),
      glm::vec3(0.1374090, -0.2333731, -0.9626296),
      glm::vec3(-0.0633145, 0.9630424, 0.2618022)};

  checkBufferContents<glm::vec3>(normalBuffer.cesium.data, expectedNormals);
}

TEST_CASE("Converts point cloud with oct-encoded normals to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "PointCloud" / "pointCloudNormalsOctEncoded.pnts";
  const int32_t pointsLength = 8;
  const int32_t expectedAttributeCount = 3;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

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
  CHECK(!material.hasExtension<ExtensionKhrMaterialsUnlit>());

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position, color, and normal attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::u8vec3>(gltf, primitive, "COLOR_0", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "NORMAL", pointsLength);

  // Check normal attribute more thoroughly
  uint32_t normalAccessorId = static_cast<uint32_t>(attributes.at("NORMAL"));
  Accessor& normalAccessor = gltf.accessors[normalAccessorId];
  CHECK(!normalAccessor.normalized);

  uint32_t normalBufferViewId =
      static_cast<uint32_t>(normalAccessor.bufferView);
  BufferView& normalBufferView = gltf.bufferViews[normalBufferViewId];

  uint32_t normalBufferId = static_cast<uint32_t>(normalBufferView.buffer);
  Buffer& normalBuffer = gltf.buffers[normalBufferId];

  const std::vector<glm::vec3> expectedNormals = {
      glm::vec3(-0.9856477, 0.1634960, 0.0420418),
      glm::vec3(-0.5901730, 0.5359042, 0.6037402),
      glm::vec3(-0.5674310, -0.7817938, -0.2584963),
      glm::vec3(-0.5861990, -0.7179291, 0.3754308),
      glm::vec3(-0.8519385, -0.1283743, -0.5076620),
      glm::vec3(0.7587127, 0.1254564, 0.6392304),
      glm::vec3(0.1354662, -0.2292506, -0.9638947),
      glm::vec3(-0.0656172, 0.9640687, 0.2574214)};

  checkBufferContents<glm::vec3>(normalBuffer.cesium.data, expectedNormals);
}

std::set<int32_t>
getUniqueBufferIds(const std::vector<BufferView>& bufferViews) {
  std::set<int32_t> result;
  for (auto it = bufferViews.begin(); it != bufferViews.end(); it++) {
    result.insert(it->buffer);
  }

  return result;
}

TEST_CASE(
    "Converts point cloud with batch IDs to glTF with EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudBatched.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  // The correctness of the model extension is thoroughly tested in
  // TestUpgradeBatchTableToExtFeatureMetadata
  CHECK(gltf.hasExtension<ExtensionModelExtFeatureMetadata>());

  CHECK(gltf.nodes.size() == 1);
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  auto primitiveExtension =
      primitive.getExtension<ExtensionMeshPrimitiveExtFeatureMetadata>();
  REQUIRE(primitiveExtension);
  REQUIRE(primitiveExtension->featureIdAttributes.size() == 1);
  FeatureIDAttribute& attribute = primitiveExtension->featureIdAttributes[0];
  CHECK(attribute.featureTable == "default");
  CHECK(attribute.featureIds.attribute == "_FEATURE_ID_0");

  CHECK(gltf.materials.size() == 1);

  // The file has three metadata properties:
  // - "name": string scalars in JSON
  // - "dimensions": float vec3s in binary
  // - "id": int scalars in binary
  // There are three accessors (one per primitive attribute)
  // and four additional buffer views:
  // - "name" string data buffer view
  // - "name" string offsets buffer view
  // - "dimensions" buffer view
  // - "id" buffer view
  REQUIRE(gltf.accessors.size() == 3);
  REQUIRE(gltf.bufferViews.size() == 7);

  // There are also three added buffers:
  // - binary data in the batch table
  // - string data of "name"
  // - string offsets for the data for "name"
  REQUIRE(gltf.buffers.size() == 6);
  std::set<int32_t> bufferSet = getUniqueBufferIds(gltf.bufferViews);
  CHECK(bufferSet.size() == 6);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == 3);

  // Check that position, normal, and feature ID attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "NORMAL", pointsLength);
  checkAttribute<uint8_t>(gltf, primitive, "_FEATURE_ID_0", pointsLength);

  // Check feature ID attribute more thoroughly
  uint32_t featureIdAccessorId =
      static_cast<uint32_t>(attributes.at("_FEATURE_ID_0"));
  Accessor& featureIdAccessor = gltf.accessors[featureIdAccessorId];

  uint32_t featureIdBufferViewId =
      static_cast<uint32_t>(featureIdAccessor.bufferView);
  BufferView& featureIdBufferView = gltf.bufferViews[featureIdBufferViewId];

  uint32_t featureIdBufferId =
      static_cast<uint32_t>(featureIdBufferView.buffer);
  Buffer& featureIdBuffer = gltf.buffers[featureIdBufferId];

  const std::vector<uint8_t> expectedFeatureIDs = {5, 5, 6, 6, 7, 0, 3, 1};
  checkBufferContents<uint8_t>(featureIdBuffer.cesium.data, expectedFeatureIDs);
}

TEST_CASE("Converts point cloud with per-point properties to glTF with "
          "EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "PointCloud" / "pointCloudWithPerPointProperties.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  // The correctness of the model extension is thoroughly tested in
  // TestUpgradeBatchTableToExtFeatureMetadata
  CHECK(gltf.hasExtension<ExtensionModelExtFeatureMetadata>());

  CHECK(gltf.nodes.size() == 1);
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  auto primitiveExtension =
      primitive.getExtension<ExtensionMeshPrimitiveExtFeatureMetadata>();
  REQUIRE(primitiveExtension);
  REQUIRE(primitiveExtension->featureIdAttributes.size() == 1);
  FeatureIDAttribute& attribute = primitiveExtension->featureIdAttributes[0];
  CHECK(attribute.featureTable == "default");
  // Check for implicit feature IDs
  CHECK(!attribute.featureIds.attribute);
  CHECK(attribute.featureIds.constant == 0);
  CHECK(attribute.featureIds.divisor == 1);

  CHECK(gltf.materials.size() == 1);

  // The file has three binary metadata properties:
  // - "temperature": float scalars
  // - "secondaryColor": float vec3s
  // - "id": unsigned short scalars
  // There are two accessors (one per primitive attribute)
  // and three additional buffer views:
  // - temperature buffer view
  // - secondary color buffer view
  // - id buffer view
  REQUIRE(gltf.accessors.size() == 2);
  REQUIRE(gltf.bufferViews.size() == 5);

  // There is only one added buffer containing all the binary values.
  REQUIRE(gltf.buffers.size() == 3);
  std::set<int32_t> bufferSet = getUniqueBufferIds(gltf.bufferViews);
  CHECK(bufferSet.size() == 3);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == 2);
  REQUIRE(attributes.find("_FEATURE_ID_0") == attributes.end());

  // Check that position and color  attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::u8vec3>(gltf, primitive, "COLOR_0", pointsLength);
}
