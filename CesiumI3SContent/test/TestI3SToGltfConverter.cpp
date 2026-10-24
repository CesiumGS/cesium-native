#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/GeometrySchema.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3SContent/I3SConverterResult.h>
#include <CesiumI3SContent/I3SToGltfConverter.h>

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace CesiumI3SContent;
using namespace CesiumGltf;
using namespace CesiumI3S;

namespace {

// Build a minimal legacy binary buffer for a single triangle (3 vertices,
// 1 feature). Layout:
//   [vertexCount: uint32][featureCount: uint32]
//   [position: float32[9]]  (3 x xyz in degrees/meters)
//   [faceRange: uint32[2]]  (startFace=0, endFace=0)
std::vector<std::byte> buildLegacyTriangleBuffer() {
  struct Header {
    uint32_t vertexCount = 3;
    uint32_t featureCount = 1;
  };
  // 3 vertices, each at a slightly different lon/lat offset from center.
  // Positions are [lon_deg_offset, lat_deg_offset, height_m_offset].
  float positions[9] = {
      0.0f,
      0.0f,
      0.0f, // vertex 0
      0.001f,
      0.0f,
      0.0f, // vertex 1
      0.0f,
      0.001f,
      0.0f, // vertex 2
  };
  // Face range: feature 0 covers face 0 (vertices 0,1,2).
  uint32_t faceRange[2] = {0, 0};

  std::vector<std::byte> buf;
  Header hdr;
  buf.resize(sizeof(Header) + sizeof(positions) + sizeof(faceRange));

  size_t off = 0;
  std::memcpy(buf.data() + off, &hdr, sizeof(hdr));
  off += sizeof(hdr);
  std::memcpy(buf.data() + off, positions, sizeof(positions));
  off += sizeof(positions);
  std::memcpy(buf.data() + off, faceRange, sizeof(faceRange));

  return buf;
}

GeometrySchema buildLegacySchema() {
  GeometrySchema schema;
  schema.header.push_back({"vertexCount", DataType::UInt32});
  schema.header.push_back({"featureCount", DataType::UInt32});
  schema.ordering = {CesiumI3S::GeometrySchema::AttributeField::Position};
  schema.featureAttributeOrder = {"faceRange"};
  return schema;
}

I3SToGltfConverterInput
makeInput(std::span<const std::byte> bytes, const GeometrySchema* pSchema) {
  I3SToGltfConverterInput input;
  input.geometryBytes = bytes;
  input.pDefaultSchema = pSchema;
  input.nodeCenterLonLatDegHeight = {0.0, 0.0, 0.0};
  // WGS84 geographic WKID 4326
  input.spatialReference.wkid = 4326;
  return input;
}

I3SToGltfConverterInput makeModernInput(
    std::span<const std::byte> bytes,
    const GeometryBuffer* pGeometryBuffer) {
  I3SToGltfConverterInput input;
  input.geometryBytes = bytes;
  input.pGeometryBuffer = pGeometryBuffer;
  input.nodeCenterLonLatDegHeight = {0.0, 0.0, 0.0};
  input.spatialReference.wkid = 4326;
  return input;
}

// Modern binary buffer: [vertexCount: u32][featureCount: u32][attributes...]
// Attributes are laid out in the fixed decoder order, each present slot
// contributing its data contiguously.
std::vector<std::byte> buildModernBuffer(
    uint32_t vertexCount,
    uint32_t featureCount,
    bool withNormals,
    bool withFaceRange) {
  std::vector<std::byte> buf;

  auto appendU32 = [&](uint32_t v) {
    std::byte bytes[4];
    std::memcpy(bytes, &v, 4);
    buf.insert(buf.end(), bytes, bytes + 4);
  };
  auto appendF32 = [&](float v) {
    std::byte bytes[4];
    std::memcpy(bytes, &v, 4);
    buf.insert(buf.end(), bytes, bytes + 4);
  };

  appendU32(vertexCount);
  appendU32(featureCount);

  // positions: slight lon/lat offsets so coordinate transform produces ENU
  for (uint32_t i = 0; i < vertexCount; ++i) {
    appendF32(static_cast<float>(i) * 0.001f); // lon offset
    appendF32(0.0f);                           // lat offset
    appendF32(0.0f);                           // height offset
  }

  if (withNormals) {
    for (uint32_t i = 0; i < vertexCount; ++i) {
      appendF32(0.0f);
      appendF32(0.0f);
      appendF32(1.0f); // up normals
    }
  }

  if (withFaceRange) {
    // One faceRange entry per feature: [startFace, endFace] both uint32
    for (uint32_t f = 0; f < featureCount; ++f) {
      appendU32(f);
      appendU32(f);
    }
  }

  return buf;
}

} // anonymous namespace

TEST_SUITE("I3SToGltfConverter") {

  TEST_CASE("Unsupported horizontal CRS returns an error") {
    GeometrySchema schema = buildLegacySchema();
    auto buf = buildLegacyTriangleBuffer();
    auto input = makeInput(buf, &schema);
    input.spatialReference.wkid = 32633; // UTM Zone 33N — not supported

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    CHECK(!result.model.has_value());
    CHECK(!result.errors.errors.empty());
  }

  TEST_CASE("Missing horizontal CRS is treated as WGS84 (succeeds)") {
    GeometrySchema schema = buildLegacySchema();
    auto buf = buildLegacyTriangleBuffer();
    auto input = makeInput(buf, &schema);
    input.spatialReference = {}; // no WKID set

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    CHECK(result.errors.errors.empty());
    REQUIRE(result.model.has_value());
  }

  TEST_CASE("Legacy binary: single triangle produces valid POSITION accessor") {
    GeometrySchema schema = buildLegacySchema();
    auto buf = buildLegacyTriangleBuffer();
    auto input = makeInput(buf, &schema);

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    REQUIRE(result.errors.errors.empty());
    REQUIRE(result.model.has_value());

    const Model& model = *result.model;
    REQUIRE(!model.meshes.empty());
    REQUIRE(!model.meshes[0].primitives.empty());

    const MeshPrimitive& prim = model.meshes[0].primitives[0];
    REQUIRE(prim.attributes.count("POSITION") == 1);

    const int32_t posAccIdx = prim.attributes.at("POSITION");
    REQUIRE(posAccIdx >= 0);
    REQUIRE(static_cast<size_t>(posAccIdx) < model.accessors.size());

    const Accessor& posAcc = model.accessors[static_cast<size_t>(posAccIdx)];
    CHECK(posAcc.count == 3);
    CHECK(posAcc.componentType == Accessor::ComponentType::FLOAT);
    CHECK(posAcc.type == Accessor::Type::VEC3);
    // min/max must be populated
    CHECK(posAcc.min.size() == 3);
    CHECK(posAcc.max.size() == 3);
  }

  TEST_CASE("Legacy binary: feature IDs produce EXT_mesh_features") {
    GeometrySchema schema = buildLegacySchema();
    auto buf = buildLegacyTriangleBuffer();
    auto input = makeInput(buf, &schema);

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    REQUIRE(result.model.has_value());
    const Model& model = *result.model;
    REQUIRE(!model.meshes.empty());
    const MeshPrimitive& prim = model.meshes[0].primitives[0];

    // _FEATURE_ID_0 attribute must be present
    CHECK(prim.attributes.count("_FEATURE_ID_0") == 1);

    // EXT_mesh_features extension must be present on the primitive
    const auto* pFeatExt = prim.getExtension<ExtensionExtMeshFeatures>();
    REQUIRE(pFeatExt != nullptr);
    REQUIRE(!pFeatExt->featureIds.empty());
    CHECK(pFeatExt->featureIds[0].featureCount == 1);
  }

  TEST_CASE("Geoid correction skipped when pGeoidGrid is null") {
    // This test simply checks the conversion succeeds when heightModel is
    // GravityRelatedHeight but pGeoidGrid is null (grid pointer check).
    GeometrySchema schema = buildLegacySchema();
    auto buf = buildLegacyTriangleBuffer();
    auto input = makeInput(buf, &schema);
    input.pGeoidGrid = nullptr; // explicitly null
    HeightModelInfo hmi;
    hmi.heightModel = HeightModel::GravityRelatedHeight;
    input.heightModelInfo = hmi;

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    CHECK(result.errors.errors.empty());
    CHECK(result.model.has_value());
  }

  TEST_CASE("No geometry schema returns an error") {
    auto buf = buildLegacyTriangleBuffer();

    I3SToGltfConverterInput input;
    input.geometryBytes = buf;
    input.pGeometryBuffer = nullptr;
    input.pDefaultSchema = nullptr;
    input.spatialReference.wkid = 4326;

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    CHECK(!result.model.has_value());
    CHECK(!result.errors.errors.empty());
  }

  TEST_CASE("UV region crop adjusts texture coordinates") {
    // Build a buffer with positions + UV coordinates + UV regions.
    struct Header {
      uint32_t vertexCount = 3;
      uint32_t featureCount = 0;
    };
    float positions[9] = {0, 0, 0, 0.001f, 0, 0, 0, 0.001f, 0};
    float uv0[6] = {1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f}; // will be cropped
    // Region: half-UV (0 to 32767 = 0 to 0.5 in U; 0 to 65535 = 0 to 1 in V)
    uint16_t regions[12] =
        {0, 0, 32767, 65535, 0, 0, 32767, 65535, 0, 0, 32767, 65535};

    std::vector<std::byte> buf;
    buf.resize(
        sizeof(Header) + sizeof(positions) + sizeof(uv0) + sizeof(regions));
    size_t off = 0;
    Header hdr;
    std::memcpy(buf.data() + off, &hdr, sizeof(hdr));
    off += sizeof(hdr);
    std::memcpy(buf.data() + off, positions, sizeof(positions));
    off += sizeof(positions);
    std::memcpy(buf.data() + off, uv0, sizeof(uv0));
    off += sizeof(uv0);
    std::memcpy(buf.data() + off, regions, sizeof(regions));

    GeometrySchema schema;
    schema.header.push_back({"vertexCount", DataType::UInt32});
    schema.header.push_back({"featureCount", DataType::UInt32});
    schema.ordering = {
        CesiumI3S::GeometrySchema::AttributeField::Position,
        CesiumI3S::GeometrySchema::AttributeField::Uv0,
        CesiumI3S::GeometrySchema::AttributeField::Region};

    auto input = makeInput(buf, &schema);

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    REQUIRE(result.errors.errors.empty());
    REQUIRE(result.model.has_value());

    const Model& model = *result.model;
    const MeshPrimitive& prim = model.meshes[0].primitives[0];
    REQUIRE(prim.attributes.count("TEXCOORD_0") == 1);

    // After UV crop: u_final = u * (32767/65535) + 0 ~ u * 0.5
    // For vertex 0: original u=1.0 -> cropped u ~ 0.5
    const int32_t uvAccIdx = prim.attributes.at("TEXCOORD_0");
    const Accessor& uvAcc = model.accessors[static_cast<size_t>(uvAccIdx)];
    REQUIRE(uvAcc.bufferView >= 0);
    const auto& bv = model.bufferViews[static_cast<size_t>(uvAcc.bufferView)];
    const auto& buf2 = model.buffers[static_cast<size_t>(bv.buffer)];
    const float* pUv = reinterpret_cast<const float*>(buf2.cesium.data.data());
    // The first UV's U component should be approximately 0.5 (not 1.0).
    CHECK(pUv[0] < 0.6f);
    CHECK(pUv[0] > 0.4f);
  }

  TEST_CASE("Modern binary: position-only produces valid POSITION accessor") {
    GeometryBuffer gb;
    gb.offset = 8;
    gb.position.component = 3;

    auto buf = buildModernBuffer(3, 0, false, false);
    auto input = makeModernInput(buf, &gb);

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    REQUIRE(result.errors.errors.empty());
    REQUIRE(result.model.has_value());

    const Model& model = *result.model;
    REQUIRE(!model.meshes.empty());
    const MeshPrimitive& prim = model.meshes[0].primitives[0];
    REQUIRE(prim.attributes.count("POSITION") == 1);

    const Accessor& posAcc =
        model.accessors[static_cast<size_t>(prim.attributes.at("POSITION"))];
    CHECK(posAcc.count == 3);
    CHECK(posAcc.type == Accessor::Type::VEC3);
    CHECK(posAcc.componentType == Accessor::ComponentType::FLOAT);
    CHECK(posAcc.min.size() == 3);
    CHECK(posAcc.max.size() == 3);
    // No normals, UVs, or feature IDs expected
    CHECK(prim.attributes.count("NORMAL") == 0);
    CHECK(prim.attributes.count("TEXCOORD_0") == 0);
    CHECK(prim.attributes.count("_FEATURE_ID_0") == 0);
  }

  TEST_CASE("Modern binary: normals produce NORMAL accessor") {
    GeometryBuffer gb;
    gb.offset = 8;
    gb.position.component = 3;
    gb.normal.component = 3;

    auto buf = buildModernBuffer(3, 0, true, false);
    auto input = makeModernInput(buf, &gb);

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    REQUIRE(result.errors.errors.empty());
    REQUIRE(result.model.has_value());

    const Model& model = *result.model;
    const MeshPrimitive& prim = model.meshes[0].primitives[0];
    REQUIRE(prim.attributes.count("NORMAL") == 1);

    const Accessor& normAcc =
        model.accessors[static_cast<size_t>(prim.attributes.at("NORMAL"))];
    CHECK(normAcc.count == 3);
    CHECK(normAcc.type == Accessor::Type::VEC3);
    CHECK(normAcc.componentType == Accessor::ComponentType::FLOAT);
  }

  TEST_CASE("Modern binary: faceRange produces EXT_mesh_features") {
    GeometryBuffer gb;
    gb.offset = 8;
    gb.position.component = 3;
    gb.faceRange.component = 2;

    // 3 vertices, 1 feature covering face 0
    auto buf = buildModernBuffer(3, 1, false, true);
    auto input = makeModernInput(buf, &gb);

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    REQUIRE(result.errors.errors.empty());
    REQUIRE(result.model.has_value());

    const Model& model = *result.model;
    const MeshPrimitive& prim = model.meshes[0].primitives[0];
    CHECK(prim.attributes.count("_FEATURE_ID_0") == 1);

    const auto* pFeatExt = prim.getExtension<ExtensionExtMeshFeatures>();
    REQUIRE(pFeatExt != nullptr);
    REQUIRE(!pFeatExt->featureIds.empty());
    CHECK(pFeatExt->featureIds[0].featureCount == 1);
  }

  TEST_CASE("Modern binary: truncated buffer returns an error") {
    GeometryBuffer gb;
    gb.offset = 8;
    gb.position.component = 3;

    // Only the header bytes — not enough room for 3 positions
    std::vector<std::byte> buf(8);
    uint32_t v = 3, f = 0;
    std::memcpy(buf.data(), &v, 4);
    std::memcpy(buf.data() + 4, &f, 4);

    auto input = makeModernInput(buf, &gb);

    I3SConverterResult result = I3SToGltfConverter::convert(input);

    CHECK(!result.model.has_value());
    CHECK(!result.errors.errors.empty());
  }

} // TEST_SUITE
