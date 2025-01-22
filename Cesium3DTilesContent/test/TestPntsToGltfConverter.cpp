#include "ConvertTileToGltf.h"

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionKhrMaterialsUnlit.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/ext/vector_uint4_sized.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

using namespace doctest;
using namespace CesiumGltf;
using namespace Cesium3DTilesContent;
using namespace CesiumUtility;

template <typename Type>
static void checkBufferContents(
    const std::vector<std::byte>& buffer,
    const std::vector<Type>& expected,
    [[maybe_unused]] const double epsilon = Math::Epsilon6) {
  REQUIRE(buffer.size() == expected.size() * sizeof(Type));
  const int32_t byteStride = sizeof(Type);
  if constexpr (std::is_same_v<Type, glm::vec3>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const glm::vec3& value =
          *reinterpret_cast<const glm::vec3*>(buffer.data() + i * byteStride);
      const glm::vec3& expectedValue = expected[i];
      REQUIRE(Math::equalsEpsilon(
          static_cast<glm::dvec3>(value),
          static_cast<glm::dvec3>(expectedValue),
          epsilon));
    }
  } else if constexpr (std::is_same_v<Type, glm::vec4>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const glm::vec4& value =
          *reinterpret_cast<const glm::vec4*>(buffer.data() + i * byteStride);
      const glm::vec4& expectedValue = expected[i];
      REQUIRE(Math::equalsEpsilon(
          static_cast<glm::dvec4>(value),
          static_cast<glm::dvec4>(expectedValue),
          epsilon));
    }
  } else if constexpr (std::is_floating_point_v<Type>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const Type& value =
          *reinterpret_cast<const Type*>(buffer.data() + i * byteStride);
      const Type& expectedValue = expected[i];
      REQUIRE(value == Approx(expectedValue));
    }
  } else if constexpr (
      std::is_integral_v<Type> || std::is_same_v<Type, glm::u8vec2> ||
      std::is_same_v<Type, glm::u8vec3> || std::is_same_v<Type, glm::u8vec4>) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const Type& value =
          *reinterpret_cast<const Type*>(buffer.data() + i * byteStride);
      const Type& expectedValue = expected[i];
      REQUIRE(value == expectedValue);
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
  } else if constexpr (std::is_same_v<Type, glm::vec4>) {
    expectedComponentType = Accessor::ComponentType::FLOAT;
    expectedType = Accessor::Type::VEC4;
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

  CHECK(gltf.asset.version == "2.0");

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

  // Check for a default scene referencing the node
  CHECK(gltf.scene == 0);
  REQUIRE(gltf.scenes.size() == 1);
  REQUIRE(gltf.scenes[0].nodes.size() == 1);
  CHECK(gltf.scenes[0].nodes[0] == 0);

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
  CHECK(gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

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
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));
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
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.alphaMode == Material::AlphaMode::BLEND);
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());
  CHECK(gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position and color attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec4>(gltf, primitive, "COLOR_0", pointsLength);

  // Check color attribute more thoroughly
  uint32_t colorAccessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(!colorAccessor.normalized);

  uint32_t colorBufferViewId = static_cast<uint32_t>(colorAccessor.bufferView);
  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];

  uint32_t colorBufferId = static_cast<uint32_t>(colorBufferView.buffer);
  Buffer& colorBuffer = gltf.buffers[colorBufferId];

  const std::vector<glm::vec4> expectedColors = {
      glm::vec4(0.263174f, 0.315762f, 0.476177f, 0.423529f),
      glm::vec4(0.325036f, 0.708297f, 0.259027f, 0.423529f),
      glm::vec4(0.151058f, 0.353740f, 0.378676f, 0.192156f),
      glm::vec4(0.160443f, 0.067724f, 0.774227f, 0.027450f),
      glm::vec4(0.915750f, 0.056374f, 0.119264f, 0.239215f),
      glm::vec4(0.592438f, 0.632042f, 0.242796f, 0.239215f),
      glm::vec4(0.284452f, 0.127529f, 0.843369f, 0.419607f),
      glm::vec4(0.002932f, 0.091518f, 0.004559f, 0.321568f)};

  checkBufferContents<glm::vec4>(colorBuffer.cesium.data, expectedColors);
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
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.alphaMode == Material::AlphaMode::OPAQUE);
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());
  CHECK(gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position and color attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);

  // Check color attribute more thoroughly
  uint32_t colorAccessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(!colorAccessor.normalized);

  uint32_t colorBufferViewId = static_cast<uint32_t>(colorAccessor.bufferView);
  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];

  uint32_t colorBufferId = static_cast<uint32_t>(colorBufferView.buffer);
  Buffer& colorBuffer = gltf.buffers[colorBufferId];

  const std::vector<glm::vec3> expectedColors = {
      glm::vec3(0.263174f, 0.315762f, 0.476177f),
      glm::vec3(0.325036f, 0.708297f, 0.259027f),
      glm::vec3(0.151058f, 0.353740f, 0.378676f),
      glm::vec3(0.160443f, 0.067724f, 0.774227f),
      glm::vec3(0.915750f, 0.056374f, 0.119264f),
      glm::vec3(0.592438f, 0.632042f, 0.242796f),
      glm::vec3(0.284452f, 0.127529f, 0.843369f),
      glm::vec3(0.002932f, 0.091518f, 0.004559f)};

  checkBufferContents<glm::vec3>(colorBuffer.cesium.data, expectedColors);
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
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.alphaMode == Material::AlphaMode::OPAQUE);
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());
  CHECK(gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position and color attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);

  // Check color attribute more thoroughly
  uint32_t colorAccessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
  Accessor& colorAccessor = gltf.accessors[colorAccessorId];
  CHECK(!colorAccessor.normalized);

  uint32_t colorBufferViewId = static_cast<uint32_t>(colorAccessor.bufferView);
  BufferView& colorBufferView = gltf.bufferViews[colorBufferViewId];

  uint32_t colorBufferId = static_cast<uint32_t>(colorBufferView.buffer);
  Buffer& colorBuffer = gltf.buffers[colorBufferId];

  const std::vector<glm::vec3> expectedColors = {
      glm::vec3(0.2666808f, 0.3100948f, 0.4702556f),
      glm::vec3(0.3024152f, 0.7123886f, 0.2333824f),
      glm::vec3(0.1478017f, 0.3481712f, 0.3813029f),
      glm::vec3(0.1478017f, 0.0635404f, 0.7379118f),
      glm::vec3(0.8635347f, 0.0560322f, 0.1023452f),
      glm::vec3(0.5694675f, 0.6282104f, 0.2333824f),
      glm::vec3(0.2666808f, 0.1196507f, 0.7993773f),
      glm::vec3(0.0024058f, 0.0891934f, 0.0024058f)};

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
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);
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
  CHECK(gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));
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
  CHECK(!gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(!gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(material.hasExtension<ExtensionKhrMaterialsUnlit>());
  CHECK(gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position and color attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);

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
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(!material.hasExtension<ExtensionKhrMaterialsUnlit>());
  CHECK(!gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position, color, and normal attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);
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
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);

  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(!material.hasExtension<ExtensionKhrMaterialsUnlit>());
  CHECK(!gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

  REQUIRE(gltf.accessors.size() == expectedAttributeCount);
  REQUIRE(gltf.bufferViews.size() == expectedAttributeCount);
  REQUIRE(gltf.buffers.size() == expectedAttributeCount);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == expectedAttributeCount);

  // Check that position, color, and normal attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);
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

TEST_CASE("Converts point cloud with batch IDs to glTF with "
          "EXT_structural_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudBatched.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  // The correctness of the model extension is thoroughly tested in
  // TestUpgradeBatchTableToExtStructuralMetadata
  CHECK(gltf.hasExtension<ExtensionModelExtStructuralMetadata>());

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  auto primitiveExtension = primitive.getExtension<ExtensionExtMeshFeatures>();
  REQUIRE(primitiveExtension);
  REQUIRE(primitiveExtension->featureIds.size() == 1);
  FeatureId& featureId = primitiveExtension->featureIds[0];
  CHECK(featureId.featureCount == 8);
  CHECK(featureId.attribute == 0);
  CHECK(featureId.propertyTable == 0);

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
          "EXT_structural_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "PointCloud" / "pointCloudWithPerPointProperties.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  // The correctness of the model extension is thoroughly tested in
  // TestUpgradeBatchTableToExtStructuralMetadata
  CHECK(gltf.hasExtension<ExtensionModelExtStructuralMetadata>());

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  auto primitiveExtension = primitive.getExtension<ExtensionExtMeshFeatures>();
  REQUIRE(primitiveExtension);
  REQUIRE(primitiveExtension->featureIds.size() == 1);
  FeatureId& featureId = primitiveExtension->featureIds[0];
  // Check for implicit feature IDs
  CHECK(featureId.featureCount == pointsLength);
  CHECK(!featureId.attribute);
  CHECK(featureId.propertyTable == 0);

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
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);
}

TEST_CASE("Converts point cloud with Draco compression to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudDraco.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  CHECK(gltf.hasExtension<CesiumGltf::ExtensionCesiumRTC>());
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));
  // The correctness of the model extension is thoroughly tested in
  // TestUpgradeBatchTableToExtStructuralMetadata
  CHECK(gltf.hasExtension<ExtensionModelExtStructuralMetadata>());

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  auto primitiveExtension = primitive.getExtension<ExtensionExtMeshFeatures>();
  REQUIRE(primitiveExtension);
  REQUIRE(primitiveExtension->featureIds.size() == 1);
  FeatureId& featureId = primitiveExtension->featureIds[0];
  // Check for implicit feature IDs
  CHECK(featureId.featureCount == pointsLength);
  CHECK(!featureId.attribute);
  CHECK(featureId.propertyTable == 0);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(!material.hasExtension<ExtensionKhrMaterialsUnlit>());
  CHECK(!gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

  // The file has three binary metadata properties:
  // - "temperature": float scalars
  // - "secondaryColor": float vec3s
  // - "id": unsigned short scalars
  // There are three accessors (one per primitive attribute)
  // and three additional buffer views:
  // - temperature buffer view
  // - secondary color buffer view
  // - id buffer view
  REQUIRE(gltf.accessors.size() == 3);
  REQUIRE(gltf.bufferViews.size() == 6);

  // There is only one added buffer containing all the binary values.
  REQUIRE(gltf.buffers.size() == 4);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == 3);

  // Check that position, color, and normal attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "NORMAL", pointsLength);

  // Check each attribute more thoroughly
  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("POSITION"));
    Accessor& accessor = gltf.accessors[accessorId];

    const glm::vec3 expectedMin(-4.9270443f, -3.9144449f, -4.8131480f);
    CHECK(accessor.min[0] == Approx(expectedMin.x));
    CHECK(accessor.min[1] == Approx(expectedMin.y));
    CHECK(accessor.min[2] == Approx(expectedMin.z));

    const glm::vec3 expectedMax(3.7791683f, 4.8152132f, 3.2142156f);
    CHECK(accessor.max[0] == Approx(expectedMax.x));
    CHECK(accessor.max[1] == Approx(expectedMax.y));
    CHECK(accessor.max[2] == Approx(expectedMax.z));

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    std::vector<glm::vec3> expected = {
        glm::vec3(-4.9270443f, 0.8337686f, 0.1705846f),
        glm::vec3(-2.9789500f, 2.6891474f, 2.9824265f),
        glm::vec3(-2.8329495f, -3.9144449f, -1.2851576f),
        glm::vec3(-2.9022198f, -3.6128526f, 1.8772986f),
        glm::vec3(-4.2673778f, -0.6459517f, -2.5240305f),
        glm::vec3(3.7791683f, 0.6222278f, 3.2142156f),
        glm::vec3(0.6870481f, -1.1670776f, -4.8131480f),
        glm::vec3(-0.3168385f, 4.8152132f, 1.3087492f),
    };
    checkBufferContents<glm::vec3>(buffer.cesium.data, expected);
  }

  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
    Accessor& accessor = gltf.accessors[accessorId];
    CHECK(!accessor.normalized);

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    std::vector<glm::vec3> expected = {
        glm::vec3(0.4761772f, 0.6870308f, 0.3250369f),
        glm::vec3(0.1510580f, 0.3537409f, 0.3786762f),
        glm::vec3(0.7742273f, 0.0016869f, 0.9157501f),
        glm::vec3(0.5924380f, 0.6320426f, 0.2427963f),
        glm::vec3(0.8433697f, 0.6730490f, 0.0029323f),
        glm::vec3(0.0001751f, 0.1087111f, 0.6661169f),
        glm::vec3(0.7299188f, 0.7299188f, 0.9489649f),
        glm::vec3(0.1801442f, 0.2348952f, 0.5795466f),
    };
    checkBufferContents<glm::vec3>(buffer.cesium.data, expected);
  }

  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("NORMAL"));
    Accessor& accessor = gltf.accessors[accessorId];

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    // The Draco-decoded normals are slightly different from the values
    // derived by manually decoding the uncompressed oct-encoded normals,
    // hence the less precise comparison.
    std::vector<glm::vec3> expected{
        glm::vec3(-0.9824559f, 0.1803542f, 0.0474616f),
        glm::vec3(-0.5766854f, 0.5427628f, 0.6106081f),
        glm::vec3(-0.5725988f, -0.7802446f, -0.2516918f),
        glm::vec3(-0.5705807f, -0.7345407f, 0.36727036f),
        glm::vec3(-0.8560267f, -0.1281128f, -0.5008047f),
        glm::vec3(0.7647877f, 0.11264316f, 0.63435888f),
        glm::vec3(0.1301889f, -0.23434004f, -0.9633979f),
        glm::vec3(-0.0450783f, 0.9616723f, 0.2704703f),
    };
    checkBufferContents<glm::vec3>(
        buffer.cesium.data,
        expected,
        Math::Epsilon1);
  }
}

TEST_CASE("Converts point cloud with partial Draco compression to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudDracoPartial.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  CHECK(gltf.hasExtension<CesiumGltf::ExtensionCesiumRTC>());
  CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));
  CHECK(gltf.hasExtension<ExtensionModelExtStructuralMetadata>());

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  auto primitiveExtension = primitive.getExtension<ExtensionExtMeshFeatures>();
  REQUIRE(primitiveExtension);
  REQUIRE(primitiveExtension->featureIds.size() == 1);
  FeatureId& featureId = primitiveExtension->featureIds[0];
  // Check for implicit feature IDs
  CHECK(featureId.featureCount == pointsLength);
  CHECK(!featureId.attribute);
  CHECK(featureId.propertyTable == 0);

  REQUIRE(gltf.materials.size() == 1);
  Material& material = gltf.materials[0];
  CHECK(!material.hasExtension<ExtensionKhrMaterialsUnlit>());
  CHECK(!gltf.isExtensionUsed(ExtensionKhrMaterialsUnlit::ExtensionName));

  // The file has three binary metadata properties:
  // - "temperature": float scalars
  // - "secondaryColor": float vec3s
  // - "id": unsigned short scalars
  // There are three accessors (one per primitive attribute)
  // and three additional buffer views:
  // - temperature buffer view
  // - secondary color buffer view
  // - id buffer view
  REQUIRE(gltf.accessors.size() == 3);
  REQUIRE(gltf.bufferViews.size() == 6);

  // There is only one added buffer containing all the binary values.
  REQUIRE(gltf.buffers.size() == 4);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == 3);

  // Check that position, color, and normal attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "NORMAL", pointsLength);

  // Check each attribute more thoroughly
  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("POSITION"));
    Accessor& accessor = gltf.accessors[accessorId];

    const glm::vec3 expectedMin(-4.9270443f, -3.9144449f, -4.8131480f);
    CHECK(accessor.min[0] == Approx(expectedMin.x));
    CHECK(accessor.min[1] == Approx(expectedMin.y));
    CHECK(accessor.min[2] == Approx(expectedMin.z));

    const glm::vec3 expectedMax(3.7791683f, 4.8152132f, 3.2142156f);
    CHECK(accessor.max[0] == Approx(expectedMax.x));
    CHECK(accessor.max[1] == Approx(expectedMax.y));
    CHECK(accessor.max[2] == Approx(expectedMax.z));

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    std::vector<glm::vec3> expected = {
        glm::vec3(-4.9270443f, 0.8337686f, 0.1705846f),
        glm::vec3(-2.9789500f, 2.6891474f, 2.9824265f),
        glm::vec3(-2.8329495f, -3.9144449f, -1.2851576f),
        glm::vec3(-2.9022198f, -3.6128526f, 1.8772986f),
        glm::vec3(-4.2673778f, -0.6459517f, -2.5240305f),
        glm::vec3(3.7791683f, 0.6222278f, 3.2142156f),
        glm::vec3(0.6870481f, -1.1670776f, -4.8131480f),
        glm::vec3(-0.3168385f, 4.8152132f, 1.3087492f),
    };
    checkBufferContents<glm::vec3>(buffer.cesium.data, expected);
  }

  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
    Accessor& accessor = gltf.accessors[accessorId];
    CHECK(!accessor.normalized);

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    std::vector<glm::vec3> expected = {
        glm::vec3(0.4761772f, 0.6870308f, 0.3250369f),
        glm::vec3(0.1510580f, 0.3537409f, 0.3786762f),
        glm::vec3(0.7742273f, 0.0016869f, 0.9157501f),
        glm::vec3(0.5924380f, 0.6320426f, 0.2427963f),
        glm::vec3(0.8433697f, 0.6730490f, 0.0029323f),
        glm::vec3(0.0001751f, 0.1087111f, 0.6661169f),
        glm::vec3(0.7299188f, 0.7299188f, 0.9489649f),
        glm::vec3(0.1801442f, 0.2348952f, 0.5795466f),
    };
    checkBufferContents<glm::vec3>(buffer.cesium.data, expected);
  }

  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("NORMAL"));
    Accessor& accessor = gltf.accessors[accessorId];

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    const std::vector<glm::vec3> expected = {
        glm::vec3(-0.9854088, 0.1667507, 0.0341110),
        glm::vec3(-0.5957704, 0.5378777, 0.5964436),
        glm::vec3(-0.5666092, -0.7828890, -0.2569800),
        glm::vec3(-0.5804154, -0.7226123, 0.3754320),
        glm::vec3(-0.8535281, -0.1291752, -0.5047805),
        glm::vec3(0.7557975, 0.1243999, 0.6428800),
        glm::vec3(0.1374090, -0.2333731, -0.9626296),
        glm::vec3(-0.0633145, 0.9630424, 0.2618022)};

    checkBufferContents<glm::vec3>(buffer.cesium.data, expected);
  }
}

TEST_CASE("Converts batched point cloud with Draco compression to glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudDracoBatched.pnts";
  const int32_t pointsLength = 8;

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  // The correctness of the model extension is thoroughly tested in
  // TestUpgradeBatchTableToExtStructuralMetadata
  CHECK(gltf.hasExtension<ExtensionModelExtStructuralMetadata>());

  CHECK(gltf.scenes.size() == 1);
  CHECK(gltf.nodes.size() == 1);
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(primitive.mode == MeshPrimitive::Mode::POINTS);

  auto primitiveExtension = primitive.getExtension<ExtensionExtMeshFeatures>();
  REQUIRE(primitiveExtension);
  REQUIRE(primitiveExtension->featureIds.size() == 1);
  FeatureId& featureId = primitiveExtension->featureIds[0];
  CHECK(featureId.featureCount == 8);
  CHECK(featureId.attribute == 0);
  CHECK(featureId.propertyTable == 0);

  CHECK(gltf.materials.size() == 1);

  // The file has three metadata properties:
  // - "name": string scalars in JSON
  // - "dimensions": float vec3s in binary
  // - "id": int scalars in binary
  // There are four accessors (one per primitive attribute)
  // and four additional buffer views:
  // - "name" string data buffer view
  // - "name" string offsets buffer view
  // - "dimensions" buffer view
  // - "id" buffer view
  REQUIRE(gltf.accessors.size() == 4);
  REQUIRE(gltf.bufferViews.size() == 8);

  // There are also three added buffers:
  // - binary data in the batch table
  // - string data of "name"
  // - string offsets for the data for "name"
  REQUIRE(gltf.buffers.size() == 7);
  std::set<int32_t> bufferSet = getUniqueBufferIds(gltf.bufferViews);
  CHECK(bufferSet.size() == 7);

  auto attributes = primitive.attributes;
  REQUIRE(attributes.size() == 4);

  // Check that position, normal, and feature ID attributes are present
  checkAttribute<glm::vec3>(gltf, primitive, "POSITION", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "COLOR_0", pointsLength);
  checkAttribute<glm::vec3>(gltf, primitive, "NORMAL", pointsLength);
  checkAttribute<uint8_t>(gltf, primitive, "_FEATURE_ID_0", pointsLength);

  // Check each attribute more thoroughly
  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("POSITION"));
    Accessor& accessor = gltf.accessors[accessorId];

    const glm::vec3 expectedMin(-4.9270443f, -3.9144449f, -4.8131480f);
    CHECK(accessor.min[0] == Approx(expectedMin.x));
    CHECK(accessor.min[1] == Approx(expectedMin.y));
    CHECK(accessor.min[2] == Approx(expectedMin.z));

    const glm::vec3 expectedMax(3.7791683f, 4.8152132f, 3.2142156f);
    CHECK(accessor.max[0] == Approx(expectedMax.x));
    CHECK(accessor.max[1] == Approx(expectedMax.y));
    CHECK(accessor.max[2] == Approx(expectedMax.z));

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    std::vector<glm::vec3> expected = {
        glm::vec3(-4.9270443f, 0.8337686f, 0.1705846f),
        glm::vec3(-2.9789500f, 2.6891474f, 2.9824265f),
        glm::vec3(-2.8329495f, -3.9144449f, -1.2851576f),
        glm::vec3(-2.9022198f, -3.6128526f, 1.8772986f),
        glm::vec3(-4.2673778f, -0.6459517f, -2.5240305f),
        glm::vec3(3.7791683f, 0.6222278f, 3.2142156f),
        glm::vec3(0.6870481f, -1.1670776f, -4.8131480f),
        glm::vec3(-0.3168385f, 4.8152132f, 1.3087492f),
    };
    checkBufferContents<glm::vec3>(buffer.cesium.data, expected);
  }

  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("COLOR_0"));
    Accessor& accessor = gltf.accessors[accessorId];
    CHECK(!accessor.normalized);

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    std::vector<glm::vec3> expected = {
        glm::vec3(0.4761772f, 0.6870308f, 0.3250369f),
        glm::vec3(0.1510580f, 0.3537409f, 0.3786762f),
        glm::vec3(0.7742273f, 0.0016869f, 0.9157501f),
        glm::vec3(0.5924380f, 0.6320426f, 0.2427963f),
        glm::vec3(0.8433697f, 0.6730490f, 0.0029323f),
        glm::vec3(0.0001751f, 0.1087111f, 0.6661169f),
        glm::vec3(0.7299188f, 0.7299188f, 0.9489649f),
        glm::vec3(0.1801442f, 0.2348952f, 0.5795466f),
    };
    checkBufferContents<glm::vec3>(buffer.cesium.data, expected);
  }

  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("NORMAL"));
    Accessor& accessor = gltf.accessors[accessorId];

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    // The Draco-decoded normals are slightly different from the values
    // derived by manually decoding the uncompressed oct-encoded normals,
    // hence the less precise comparison.
    std::vector<glm::vec3> expected{
        glm::vec3(-0.9824559f, 0.1803542f, 0.0474616f),
        glm::vec3(-0.5766854f, 0.5427628f, 0.6106081f),
        glm::vec3(-0.5725988f, -0.7802446f, -0.2516918f),
        glm::vec3(-0.5705807f, -0.7345407f, 0.36727036f),
        glm::vec3(-0.8560267f, -0.1281128f, -0.5008047f),
        glm::vec3(0.7647877f, 0.11264316f, 0.63435888f),
        glm::vec3(0.1301889f, -0.23434004f, -0.9633979f),
        glm::vec3(-0.0450783f, 0.9616723f, 0.2704703f),
    };
    checkBufferContents<glm::vec3>(
        buffer.cesium.data,
        expected,
        Math::Epsilon1);
  }

  {
    uint32_t accessorId = static_cast<uint32_t>(attributes.at("_FEATURE_ID_0"));
    Accessor& accessor = gltf.accessors[accessorId];

    uint32_t bufferViewId = static_cast<uint32_t>(accessor.bufferView);
    BufferView& bufferView = gltf.bufferViews[bufferViewId];

    uint32_t bufferId = static_cast<uint32_t>(bufferView.buffer);
    Buffer& buffer = gltf.buffers[bufferId];

    const std::vector<uint8_t> expected = {5, 5, 6, 6, 7, 0, 3, 1};
    checkBufferContents<uint8_t>(buffer.cesium.data, expected);
  }
}
