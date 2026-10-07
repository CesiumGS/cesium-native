#include "I3SGltfAssembler.h"

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltf/TextureInfo.h>
#include <CesiumI3S/Material.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace CesiumI3SContent {

namespace {

/**
 * @brief Append raw bytes to `model.buffers`, add a matching BufferView, and
 * return the index of the new BufferView.
 */
int32_t addBufferView(
    CesiumGltf::Model& model,
    const void* data,
    size_t byteLength,
    std::optional<int32_t> target = std::nullopt) {
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = static_cast<int64_t>(byteLength);
  buffer.cesium.data.resize(byteLength);
  std::memcpy(buffer.cesium.data.data(), data, byteLength);

  CesiumGltf::BufferView& bv = model.bufferViews.emplace_back();
  bv.buffer = static_cast<int32_t>(model.buffers.size()) - 1;
  bv.byteOffset = 0;
  bv.byteLength = static_cast<int64_t>(byteLength);
  if (target.has_value()) {
    bv.target = *target;
  }

  return static_cast<int32_t>(model.bufferViews.size()) - 1;
}

/**
 * @brief Add a float32 VEC3 accessor for a position-style attribute.
 * Computes and stores min/max values.
 */
int32_t addVec3FloatAccessor(
    CesiumGltf::Model& model,
    const std::vector<glm::vec3>& data,
    bool computeMinMax) {
  const size_t byteLen = data.size() * sizeof(glm::vec3);
  const int32_t bvIndex = addBufferView(
      model,
      data.data(),
      byteLen,
      CesiumGltf::BufferView::Target::ARRAY_BUFFER);

  CesiumGltf::Accessor& acc = model.accessors.emplace_back();
  acc.bufferView = bvIndex;
  acc.byteOffset = 0;
  acc.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  acc.count = static_cast<int64_t>(data.size());
  acc.type = CesiumGltf::Accessor::Type::VEC3;

  if (computeMinMax && !data.empty()) {
    glm::vec3 mn = data[0];
    glm::vec3 mx = data[0];
    for (const auto& v : data) {
      mn = glm::min(mn, v);
      mx = glm::max(mx, v);
    }
    acc.min = {mn.x, mn.y, mn.z};
    acc.max = {mx.x, mx.y, mx.z};
  }

  return static_cast<int32_t>(model.accessors.size()) - 1;
}

/**
 * @brief Add a float32 VEC2 accessor (UV coordinates).
 */
int32_t addVec2FloatAccessor(
    CesiumGltf::Model& model,
    const std::vector<glm::vec2>& data) {
  const size_t byteLen = data.size() * sizeof(glm::vec2);
  const int32_t bvIndex = addBufferView(
      model,
      data.data(),
      byteLen,
      CesiumGltf::BufferView::Target::ARRAY_BUFFER);

  CesiumGltf::Accessor& acc = model.accessors.emplace_back();
  acc.bufferView = bvIndex;
  acc.byteOffset = 0;
  acc.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  acc.count = static_cast<int64_t>(data.size());
  acc.type = CesiumGltf::Accessor::Type::VEC2;

  return static_cast<int32_t>(model.accessors.size()) - 1;
}

/**
 * @brief Add a float32 VEC4 accessor for vertex colors (converted from uint8
 * sRGB by dividing by 255, matching CesiumJS behavior).
 */
int32_t addColorFloatAccessor(
    CesiumGltf::Model& model,
    const std::vector<glm::u8vec4>& data) {
  std::vector<float> floats;
  floats.reserve(data.size() * 4);
  for (const auto& c : data) {
    floats.push_back(static_cast<float>(c.x) / 255.0f);
    floats.push_back(static_cast<float>(c.y) / 255.0f);
    floats.push_back(static_cast<float>(c.z) / 255.0f);
    floats.push_back(static_cast<float>(c.w) / 255.0f);
  }

  const size_t byteLen = floats.size() * sizeof(float);
  const int32_t bvIndex = addBufferView(
      model,
      floats.data(),
      byteLen,
      CesiumGltf::BufferView::Target::ARRAY_BUFFER);

  CesiumGltf::Accessor& acc = model.accessors.emplace_back();
  acc.bufferView = bvIndex;
  acc.byteOffset = 0;
  acc.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  acc.count = static_cast<int64_t>(data.size());
  acc.type = CesiumGltf::Accessor::Type::VEC4;

  return static_cast<int32_t>(model.accessors.size()) - 1;
}

/**
 * @brief Add a float32 SCALAR accessor for feature IDs (converted from uint32,
 * matching CesiumJS behavior).
 */
int32_t addFeatureIdFloatAccessor(
    CesiumGltf::Model& model,
    const std::vector<uint32_t>& data) {
  std::vector<float> floats;
  floats.reserve(data.size());
  for (const uint32_t id : data) {
    floats.push_back(static_cast<float>(id));
  }

  const size_t byteLen = floats.size() * sizeof(float);
  const int32_t bvIndex = addBufferView(
      model,
      floats.data(),
      byteLen,
      CesiumGltf::BufferView::Target::ARRAY_BUFFER);

  CesiumGltf::Accessor& acc = model.accessors.emplace_back();
  acc.bufferView = bvIndex;
  acc.byteOffset = 0;
  acc.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  acc.count = static_cast<int64_t>(data.size());
  acc.type = CesiumGltf::Accessor::Type::SCALAR;

  return static_cast<int32_t>(model.accessors.size()) - 1;
}

/**
 * @brief Add an index accessor. Uses UINT if vertexCount > 65535, else USHORT.
 */
int32_t addIndexAccessor(
    CesiumGltf::Model& model,
    const std::vector<uint32_t>& indices,
    size_t vertexCount) {
  const bool useUInt = vertexCount > std::numeric_limits<uint16_t>::max();

  if (useUInt) {
    const size_t byteLen = indices.size() * sizeof(uint32_t);
    const int32_t bvIndex = addBufferView(
        model,
        indices.data(),
        byteLen,
        CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER);

    CesiumGltf::Accessor& acc = model.accessors.emplace_back();
    acc.bufferView = bvIndex;
    acc.byteOffset = 0;
    acc.componentType = CesiumGltf::Accessor::ComponentType::UNSIGNED_INT;
    acc.count = static_cast<int64_t>(indices.size());
    acc.type = CesiumGltf::Accessor::Type::SCALAR;
    return static_cast<int32_t>(model.accessors.size()) - 1;
  } else {
    std::vector<uint16_t> u16;
    u16.reserve(indices.size());
    for (const uint32_t idx : indices) {
      u16.push_back(static_cast<uint16_t>(idx));
    }
    const size_t byteLen = u16.size() * sizeof(uint16_t);
    const int32_t bvIndex = addBufferView(
        model,
        u16.data(),
        byteLen,
        CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER);

    CesiumGltf::Accessor& acc = model.accessors.emplace_back();
    acc.bufferView = bvIndex;
    acc.byteOffset = 0;
    acc.componentType = CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT;
    acc.count = static_cast<int64_t>(u16.size());
    acc.type = CesiumGltf::Accessor::Type::SCALAR;
    return static_cast<int32_t>(model.accessors.size()) - 1;
  }
}

int32_t buildMaterial(
    CesiumGltf::Model& model,
    const CesiumI3S::MaterialDefinition& matDef,
    int32_t baseColorTextureIndex) {
  CesiumGltf::Material& mat = model.materials.emplace_back();

  // Alpha mode
  switch (matDef.alphaMode) {
  case CesiumI3S::MaterialDefinition::AlphaMode::Mask:
    mat.alphaMode = CesiumGltf::Material::AlphaMode::MASK;
    mat.alphaCutoff = matDef.alphaCutoff;
    break;
  case CesiumI3S::MaterialDefinition::AlphaMode::Blend:
    mat.alphaMode = CesiumGltf::Material::AlphaMode::BLEND;
    break;
  default:
    mat.alphaMode = CesiumGltf::Material::AlphaMode::OPAQUE;
    break;
  }

  mat.doubleSided = matDef.doubleSided;

  // Emissive factor
  if (matDef.emissiveFactor[0] != 0.0 || matDef.emissiveFactor[1] != 0.0 ||
      matDef.emissiveFactor[2] != 0.0) {
    mat.emissiveFactor = {
        static_cast<double>(matDef.emissiveFactor[0]),
        static_cast<double>(matDef.emissiveFactor[1]),
        static_cast<double>(matDef.emissiveFactor[2])};
  }

  // PBR metallic-roughness
  if (matDef.pbrMetallicRoughness.has_value()) {
    const auto& pbr = *matDef.pbrMetallicRoughness;
    auto& gltfPbr = mat.pbrMetallicRoughness.emplace();

    gltfPbr.baseColorFactor = {
        pbr.baseColorFactor[0],
        pbr.baseColorFactor[1],
        pbr.baseColorFactor[2],
        pbr.baseColorFactor[3]};
    gltfPbr.metallicFactor = pbr.metallicFactor;
    gltfPbr.roughnessFactor = pbr.roughnessFactor;

    if (baseColorTextureIndex >= 0) {
      gltfPbr.baseColorTexture.emplace().index = baseColorTextureIndex;
    }
  } else if (baseColorTextureIndex >= 0) {
    auto& gltfPbr = mat.pbrMetallicRoughness.emplace();
    gltfPbr.baseColorTexture.emplace().index = baseColorTextureIndex;
  }

  return static_cast<int32_t>(model.materials.size()) - 1;
}

/**
 * @brief Add an image + sampler + texture for a URI-based texture.
 * @return The glTF texture index, or -1 if the URI is empty.
 */
int32_t addTexture(CesiumGltf::Model& model, const std::string& uri) {
  if (uri.empty()) {
    return -1;
  }

  // Image
  CesiumGltf::Image& img = model.images.emplace_back();
  img.uri = uri;
  const int32_t imageIndex = static_cast<int32_t>(model.images.size()) - 1;

  // Sampler
  CesiumGltf::Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = CesiumGltf::Sampler::WrapS::REPEAT;
  sampler.wrapT = CesiumGltf::Sampler::WrapT::REPEAT;
  const int32_t samplerIndex = static_cast<int32_t>(model.samplers.size()) - 1;

  // Texture
  CesiumGltf::Texture& tex = model.textures.emplace_back();
  tex.source = imageIndex;
  tex.sampler = samplerIndex;

  return static_cast<int32_t>(model.textures.size()) - 1;
}

void addFeatureExtensions(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    int32_t featureIdAccessorIndex,
    int32_t featureCount) {
  // Add a minimal property table buffer to hold per-feature data.
  // The property table maps each feature index to itself (float32).
  std::vector<float> propValues(static_cast<size_t>(featureCount));
  for (int32_t i = 0; i < featureCount; ++i) {
    propValues[static_cast<size_t>(i)] = static_cast<float>(i);
  }
  const int32_t propBvIndex = addBufferView(
      model,
      propValues.data(),
      propValues.size() * sizeof(float));

  // EXT_structural_metadata on the model
  auto& metaExt =
      model.addExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
  model.extensionsUsed.push_back(
      CesiumGltf::ExtensionModelExtStructuralMetadata::ExtensionName);

  // Minimal schema: one class "feature" with one float32 property "id".
  CesiumGltf::Schema& schemaObj = metaExt.schema.emplace();
  CesiumGltf::Class& cls = schemaObj.classes["feature"];
  CesiumGltf::ClassProperty& prop = cls.properties["id"];
  prop.type = CesiumGltf::ClassProperty::Type::SCALAR;
  prop.componentType = CesiumGltf::ClassProperty::ComponentType::FLOAT32;

  // Property table
  CesiumGltf::PropertyTable& table = metaExt.propertyTables.emplace_back();
  table.classProperty = "feature";
  table.count = static_cast<int64_t>(featureCount);
  CesiumGltf::PropertyTableProperty& tableProp = table.properties["id"];
  tableProp.values = propBvIndex;

  const int32_t propertyTableIndex =
      static_cast<int32_t>(metaExt.propertyTables.size()) - 1;

  // EXT_mesh_features on the primitive
  model.extensionsUsed.push_back(
      CesiumGltf::ExtensionExtMeshFeatures::ExtensionName);
  auto& featExt =
      primitive.addExtension<CesiumGltf::ExtensionExtMeshFeatures>();

  CesiumGltf::FeatureId& fid = featExt.featureIds.emplace_back();
  fid.featureCount = static_cast<int64_t>(featureCount);
  fid.attribute = 0; // _FEATURE_ID_0
  fid.propertyTable = propertyTableIndex;

  // Add the accessor reference to the primitive attributes.
  primitive.attributes["_FEATURE_ID_0"] = featureIdAccessorIndex;
}

} // anonymous namespace

CesiumGltf::Model I3SGltfAssembler::assemble(
    const DecodedGeometry& geometry,
    const CesiumI3S::MaterialDefinition* pMaterialDef,
    const std::string& textureUri) {
  CesiumGltf::Model model;

  if (geometry.positions.empty()) {
    return model;
  }

  const size_t vertexCount = geometry.positions.size();

  // Texture
  const int32_t textureIndex = addTexture(model, textureUri);

  // Material
  int32_t materialIndex = -1;
  if (pMaterialDef != nullptr) {
    materialIndex = buildMaterial(model, *pMaterialDef, textureIndex);
  } else if (!textureUri.empty()) {
    // No explicit material; create a default one with just the texture.
    CesiumGltf::Material& mat = model.materials.emplace_back();
    auto& gltfPbr = mat.pbrMetallicRoughness.emplace();
    gltfPbr.baseColorTexture.emplace().index = textureIndex;
    materialIndex = static_cast<int32_t>(model.materials.size()) - 1;
  }

  // Mesh primitive
  CesiumGltf::MeshPrimitive primitive;
  primitive.mode = CesiumGltf::MeshPrimitive::Mode::TRIANGLES;
  if (materialIndex >= 0) {
    primitive.material = materialIndex;
  }

  // POSITION (VEC3 FLOAT, with min/max)
  primitive.attributes["POSITION"] =
      addVec3FloatAccessor(model, geometry.positions, true);

  // NORMAL (VEC3 FLOAT)
  if (!geometry.normals.empty()) {
    primitive.attributes["NORMAL"] =
        addVec3FloatAccessor(model, geometry.normals, false);
  }

  // TEXCOORD_0 (VEC2 FLOAT)
  if (!geometry.texCoords.empty()) {
    primitive.attributes["TEXCOORD_0"] =
        addVec2FloatAccessor(model, geometry.texCoords);
  }

  // COLOR_0 (VEC4 FLOAT, normalised from uint8)
  if (!geometry.colors.empty()) {
    primitive.attributes["COLOR_0"] =
        addColorFloatAccessor(model, geometry.colors);
  }

  // Indices (SCALAR UINT or USHORT)
  if (!geometry.indices.empty()) {
    primitive.indices = addIndexAccessor(model, geometry.indices, vertexCount);
  }

  // _FEATURE_ID_0 + extensions
  if (!geometry.featureIds.empty() && geometry.featureCount > 0u) {
    const int32_t fidAccessorIndex =
        addFeatureIdFloatAccessor(model, geometry.featureIds);
    addFeatureExtensions(
        model,
        primitive,
        fidAccessorIndex,
        static_cast<int32_t>(geometry.featureCount));
  }

  // Mesh
  CesiumGltf::Mesh& mesh = model.meshes.emplace_back();
  mesh.primitives.push_back(std::move(primitive));

  // Node
  CesiumGltf::Node& node = model.nodes.emplace_back();
  node.mesh = static_cast<int32_t>(model.meshes.size()) - 1;

  // Scene
  CesiumGltf::Scene& scene = model.scenes.emplace_back();
  scene.nodes.push_back(static_cast<int32_t>(model.nodes.size()) - 1);
  model.scene = static_cast<int32_t>(model.scenes.size()) - 1;

  return model;
}

} // namespace CesiumI3SContent
