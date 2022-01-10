#include "CesiumGltfReader/GltfReader.h"
#include "CesiumGltfWriter/WriteModelOptions.h"
#include "CesiumGltfWriter/Writer.h"

#include <CesiumGltf/AccessorSparseIndices.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/Scene.h>

#include <catch2/catch.hpp>
#include <gsl/span>

#include <algorithm>
#include <cstddef>
#include <string>

using namespace CesiumGltf;

// indices followed by positional data
// 3 ushorts, 2 padding bytes, then 9 floats
const std::vector<std::byte> TRIANGLE_INDICES_THEN_FLOATS{
    std::byte(0x00), std::byte(0x00), std::byte(0x01), std::byte(0x00),
    std::byte(0x02), std::byte(0x00), std::byte(0x00), std::byte(0x00),
    std::byte(0x00), std::byte(0x00), std::byte(0x00), std::byte(0x00),
    std::byte(0x00), std::byte(0x00), std::byte(0x00), std::byte(0x00),
    std::byte(0x00), std::byte(0x00), std::byte(0x00), std::byte(0x00),
    std::byte(0x00), std::byte(0x00), std::byte(0x80), std::byte(0x3f),
    std::byte(0x00), std::byte(0x00), std::byte(0x00), std::byte(0x00),
    std::byte(0x00), std::byte(0x00), std::byte(0x00), std::byte(0x00),
    std::byte(0x00), std::byte(0x00), std::byte(0x00), std::byte(0x00),
    std::byte(0x00), std::byte(0x00), std::byte(0x80), std::byte(0x3f),
    std::byte(0x00), std::byte(0x00), std::byte(0x00), std::byte(0x00)};

Model generateTriangleModel() {
  Buffer buffer;
  buffer.cesium.data = TRIANGLE_INDICES_THEN_FLOATS;
  buffer.byteLength =
      static_cast<std::int64_t>(TRIANGLE_INDICES_THEN_FLOATS.size());

  BufferView indicesBufferView;
  indicesBufferView.buffer = 0;
  indicesBufferView.byteOffset = 0;
  indicesBufferView.byteLength = 6;
  indicesBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;

  BufferView vertexBufferView;
  vertexBufferView.buffer = 0;
  vertexBufferView.byteOffset = 8;
  vertexBufferView.byteLength = 36;
  vertexBufferView.target = BufferView::Target::ARRAY_BUFFER;

  Accessor indicesAccessor;
  indicesAccessor.bufferView = 0;
  indicesAccessor.byteOffset = 0;
  indicesAccessor.componentType = AccessorSpec::ComponentType::UNSIGNED_SHORT;
  indicesAccessor.count = 3;
  indicesAccessor.type = AccessorSpec::Type::SCALAR;
  indicesAccessor.max = std::vector<double>{2.0};
  indicesAccessor.min = std::vector<double>{0.0};

  Accessor vertexAccessor;
  vertexAccessor.bufferView = 1;
  vertexAccessor.byteOffset = 0;
  vertexAccessor.componentType = AccessorSpec::ComponentType::FLOAT;
  vertexAccessor.count = 3;
  vertexAccessor.type = AccessorSpec::Type::VEC3;
  vertexAccessor.max = std::vector<double>{1.0, 1.0, 0.0};
  vertexAccessor.min = std::vector<double>{0.0, 0.0, 0.0};

  Node node;
  node.mesh = 0;

  Scene scene;
  scene.nodes.emplace_back(0);

  MeshPrimitive trianglePrimitive;
  trianglePrimitive.attributes.emplace("POSITION", 1);
  trianglePrimitive.indices = 0;

  Mesh mesh;
  mesh.primitives.emplace_back(std::move(trianglePrimitive));

  Model triangleModel;

  triangleModel.scenes.emplace_back(std::move(scene));
  triangleModel.nodes.emplace_back(std::move(node));
  triangleModel.meshes.emplace_back(std::move(mesh));
  triangleModel.buffers.emplace_back(std::move(buffer));
  triangleModel.bufferViews.emplace_back(std::move(indicesBufferView));
  triangleModel.bufferViews.emplace_back(std::move(vertexBufferView));
  triangleModel.accessors.emplace_back(std::move(indicesAccessor));
  triangleModel.accessors.emplace_back(std::move(vertexAccessor));
  triangleModel.asset.version = "2.0";
  return triangleModel;
}

TEST_CASE(
    "Generates glTF asset with required top level property `asset`",
    "[GltfWriter]") {
  CesiumGltf::Model m;
  m.asset.version = "2.0";

  CesiumGltfWriter::WriteModelOptions options;
  options.exportType = CesiumGltfWriter::GltfExportType::GLTF;

  const auto writeResult =
      CesiumGltfWriter::writeModelAsEmbeddedBytes(m, options);
  const auto asBytes = writeResult.gltfAssetBytes;
  const auto expectedString = "{\"asset\":{\"version\":\"2.0\"}}";
  const std::string asString(
      reinterpret_cast<const char*>(asBytes.data()),
      asBytes.size());
  REQUIRE(asString == expectedString);
}

TEST_CASE(
    "Generates glb asset with required top level property `asset`",
    "[GltfWriter]") {
  CesiumGltf::Model m;
  m.asset.version = "2.0";

  CesiumGltfWriter::WriteModelOptions options;
  options.exportType = CesiumGltfWriter::GltfExportType::GLB;

  const auto writeResult =
      CesiumGltfWriter::writeModelAsEmbeddedBytes(m, options);
  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  auto& asBytes = writeResult.gltfAssetBytes;
  std::vector<std::byte> expectedMagic = {
      std::byte('g'),
      std::byte('l'),
      std::byte('T'),
      std::byte('F'),
  };

  REQUIRE(
      std::equal(expectedMagic.begin(), expectedMagic.end(), asBytes.begin()));

  const std::uint32_t expectedGLBContainerVersion = 2;
  const std::int64_t actualGLBContainerVersion =
      (static_cast<std::uint8_t>(asBytes[4])) |
      (static_cast<std::uint8_t>(asBytes[5]) << 8) |
      (static_cast<std::uint8_t>(asBytes[6]) << 16) |
      (static_cast<std::uint8_t>(asBytes[7]) << 24);

  //  12 byte header + 8 bytes for JSON chunk + 27 bytes for json string + 1
  //  byte for padding to 48 bytes.
  const std::int64_t expectedGLBSize = 48;
  const std::int64_t totalGLBSize = // is 48
      (static_cast<std::uint8_t>(asBytes[8])) |
      (static_cast<std::uint8_t>(asBytes[9]) << 8) |
      (static_cast<std::uint8_t>(asBytes[10]) << 16) |
      (static_cast<std::uint8_t>(asBytes[11]) << 24);

  REQUIRE(expectedGLBContainerVersion == actualGLBContainerVersion);
  REQUIRE(expectedGLBSize == totalGLBSize);

  const std::string expectedString = "{\"asset\":{\"version\":\"2.0\"}}";
  const auto* asStringPtr = reinterpret_cast<const char*>(asBytes.data() + 20);
  std::string extractedJson(asStringPtr, expectedString.size());
  REQUIRE(expectedString == extractedJson);
}

TEST_CASE("Basic triangle is serialized to embedded glTF 2.0", "[GltfWriter]") {
  const auto validateStructure = [](const std::vector<std::byte>& gltfAsset) {
    CesiumGltfReader::GltfReader reader;
    auto loadedModelResult = reader.readModel(gsl::span(gltfAsset));
    REQUIRE(loadedModelResult.model.has_value());
    auto& loadedModel = loadedModelResult.model;

    REQUIRE(loadedModel->accessors.size() == 2);
    const auto& accessors = loadedModel->accessors;

    // Triangle Indices
    REQUIRE(accessors[0].bufferView == 0);
    REQUIRE(accessors[0].byteOffset == 0);
    REQUIRE(
        accessors[0].componentType ==
        CesiumGltf::AccessorSpec::ComponentType::UNSIGNED_SHORT);
    REQUIRE(accessors[0].count == 3);
    REQUIRE(accessors[0].min == std::vector<double>{0.0});
    REQUIRE(accessors[0].max == std::vector<double>{2.0});

    // Triangle Positions
    REQUIRE(accessors[1].bufferView == 1);
    REQUIRE(accessors[1].byteOffset == 0);
    REQUIRE(
        accessors[1].componentType ==
        CesiumGltf::AccessorSpec::ComponentType::FLOAT);
    REQUIRE(accessors[1].count == 3);
    REQUIRE(accessors[1].min == std::vector<double>{0.0, 0.0, 0.0});
    REQUIRE(accessors[1].max == std::vector<double>{1.0, 1.0, 0.0});

    // Asset
    const auto& asset = loadedModel->asset;
    REQUIRE(asset.version == "2.0");

    // Buffer
    const auto& buffers = loadedModel->buffers;
    REQUIRE(buffers.size() == 1);
    const auto& buffer = buffers.at(0);

    REQUIRE(buffer.cesium.data == TRIANGLE_INDICES_THEN_FLOATS);
    REQUIRE(buffer.cesium.data.size() == TRIANGLE_INDICES_THEN_FLOATS.size());

    // BufferViews
    const auto& bufferViews = loadedModel->bufferViews;
    REQUIRE(bufferViews.size() == 2);

    const auto indicesBufferView = bufferViews.at(0);
    REQUIRE(indicesBufferView.buffer == 0);
    REQUIRE(indicesBufferView.byteOffset == 0);
    REQUIRE(indicesBufferView.byteLength == 6);
    REQUIRE(
        (indicesBufferView.target.has_value() &&
         *indicesBufferView.target ==
             CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER));

    const auto positionBufferView = bufferViews.at(1);
    REQUIRE(positionBufferView.buffer == 0);
    REQUIRE(positionBufferView.byteOffset == 8);
    REQUIRE(positionBufferView.byteLength == 36);
    REQUIRE(
        (positionBufferView.target.has_value() &&
         *positionBufferView.target ==
             CesiumGltf::BufferView::Target::ARRAY_BUFFER));

    // Meshes
    const auto meshes = loadedModel->meshes;
    REQUIRE(meshes.size() == 1);
    const auto& mesh = meshes.at(0);
    REQUIRE(mesh.primitives.size() == 1);

    // MeshPrimitive
    const auto& primitive = mesh.primitives.at(0);
    REQUIRE(primitive.attributes.count("POSITION") == 1);
    REQUIRE(primitive.attributes.at("POSITION") == 1);
    REQUIRE(primitive.indices == 0);

    const auto& nodes = loadedModel->nodes;
    REQUIRE(nodes.size() == 1);
    const auto& node = nodes.at(0);
    REQUIRE(node.mesh == 0);

    const auto& scenes = loadedModel->scenes;
    REQUIRE(scenes.size() == 1);

    const auto& scene = scenes.at(0);
    REQUIRE(scene.nodes == std::vector<std::int32_t>{0});
  };

  const auto model = generateTriangleModel();

  CesiumGltfWriter::WriteModelOptions options;
  options.exportType = CesiumGltfWriter::GltfExportType::GLTF;
  options.autoConvertDataToBase64 = true;

  const auto writeResultGltf =
      CesiumGltfWriter::writeModelAsEmbeddedBytes(model, options);
  REQUIRE(writeResultGltf.errors.empty());
  REQUIRE(writeResultGltf.warnings.empty());
  validateStructure(writeResultGltf.gltfAssetBytes);

  options.exportType = CesiumGltfWriter::GltfExportType::GLB;
  options.autoConvertDataToBase64 = false;
  const auto writeResultGlb =
      CesiumGltfWriter::writeModelAsEmbeddedBytes(model, options);
  REQUIRE(writeResultGlb.errors.empty());
  REQUIRE(writeResultGlb.warnings.empty());
  validateStructure(writeResultGlb.gltfAssetBytes);
}
