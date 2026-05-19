#include "I3SDracoDecoder.h"

#include <CesiumUtility/ErrorList.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>

// Suppress warnings from draco headers on MSVC.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127 4018 4804)
#endif

#include <draco/attributes/geometry_attribute.h>
#include <draco/attributes/point_attribute.h>
#include <draco/compression/decode.h>
#include <draco/core/decoder_buffer.h>
#include <draco/mesh/mesh.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace CesiumI3SContent {

namespace {

// Read all float32 values for a single vec3 attribute into a vector.
std::vector<glm::vec3>
readVec3Attribute(const draco::PointAttribute& attr, int32_t vertexCount) {
  std::vector<glm::vec3> out(static_cast<size_t>(vertexCount));
  for (draco::PointIndex pi(0); pi < vertexCount; ++pi) {
    const draco::AttributeValueIndex vi = attr.mapped_index(pi);
    attr.ConvertValue<float>(vi, 3, &out[pi.value()].x);
  }
  return out;
}

// Read all float32 values for a single vec2 attribute into a vector.
std::vector<glm::vec2>
readVec2Attribute(const draco::PointAttribute& attr, int32_t vertexCount) {
  std::vector<glm::vec2> out(static_cast<size_t>(vertexCount));
  for (draco::PointIndex pi(0); pi < vertexCount; ++pi) {
    const draco::AttributeValueIndex vi = attr.mapped_index(pi);
    attr.ConvertValue<float>(vi, 2, &out[pi.value()].x);
  }
  return out;
}

// Read all uint8 RGBA values for a color attribute into a vector.
std::vector<glm::u8vec4>
readColorAttribute(const draco::PointAttribute& attr, int32_t vertexCount) {
  std::vector<glm::u8vec4> out(
      static_cast<size_t>(vertexCount),
      {255, 255, 255, 255});
  const int8_t nComp =
      static_cast<int8_t>(std::min<int>(attr.num_components(), 4));
  for (draco::PointIndex pi(0); pi < vertexCount; ++pi) {
    const draco::AttributeValueIndex vi = attr.mapped_index(pi);
    attr.ConvertValue<uint8_t>(vi, nComp, &out[pi.value()].x);
  }
  return out;
}

// Read all uint32 values for a scalar attribute (feature indices).
std::vector<uint32_t>
readUInt32Attribute(const draco::PointAttribute& attr, int32_t vertexCount) {
  std::vector<uint32_t> out(static_cast<size_t>(vertexCount), 0u);
  for (draco::PointIndex pi(0); pi < vertexCount; ++pi) {
    const draco::AttributeValueIndex vi = attr.mapped_index(pi);
    attr.ConvertValue<uint32_t>(vi, 1, &out[pi.value()]);
  }
  return out;
}

} // anonymous namespace

DecodedGeometry I3SDracoDecoder::decode(std::span<const std::byte> data) {
  DecodedGeometry result;

  // Validate Draco magic "DRACO"
  if (data.size() < 5 || static_cast<char>(data[0]) != 'D' ||
      static_cast<char>(data[1]) != 'R' || static_cast<char>(data[2]) != 'A' ||
      static_cast<char>(data[3]) != 'C' || static_cast<char>(data[4]) != 'O') {
    result.errors.emplaceError(
        "Buffer does not contain Draco-encoded data (missing magic bytes)");
    return result;
  }

  // Decode
  draco::DecoderBuffer decodeBuffer;
  decodeBuffer.Init(reinterpret_cast<const char*>(data.data()), data.size());

  draco::Decoder decoder;
  draco::StatusOr<std::unique_ptr<draco::Mesh>> decodeResult =
      decoder.DecodeMeshFromBuffer(&decodeBuffer);
  if (!decodeResult.ok()) {
    result.errors.emplaceError(
        std::string("Draco decode failed: ") +
        decodeResult.status().error_msg_string());
    return result;
  }

  std::unique_ptr<draco::Mesh> pMesh = std::move(decodeResult).value();
  const int32_t vertexCount = pMesh->num_points();
  const int32_t faceCount = pMesh->num_faces();

  if (vertexCount == 0) {
    return result;
  }

  // Decode triangle indices
  result.indices.resize(static_cast<size_t>(faceCount) * 3);
  for (draco::FaceIndex fi(0); fi < faceCount; ++fi) {
    const draco::Mesh::Face& face = pMesh->face(fi);
    result.indices[fi.value() * 3 + 0] = face[0].value();
    result.indices[fi.value() * 3 + 1] = face[1].value();
    result.indices[fi.value() * 3 + 2] = face[2].value();
  }

  // Decode vertex attributes
  bool hasFeatureIndex = false;

  for (int32_t attrId = 0; attrId < pMesh->num_attributes(); ++attrId) {
    const draco::PointAttribute* pAttr = pMesh->attribute(attrId);
    if (!pAttr) {
      continue;
    }

    const draco::GeometryAttribute::Type attrType = pAttr->attribute_type();

    // Check i3s-specific metadata to override attribute role.
    const draco::AttributeMetadata* pMeta =
        pMesh->GetAttributeMetadataByAttributeId(attrId);

    std::string i3sAttrType;
    if (pMeta) {
      pMeta->GetEntryString("i3s-attribute-type", &i3sAttrType);
      // Read coordinate scale factors from any attribute's metadata.
      double sx = 0.0;
      if (pMeta->GetEntryDouble("i3s-scale_x", &sx)) {
        result.scaleX = sx;
      }
      double sy = 0.0;
      if (pMeta->GetEntryDouble("i3s-scale_y", &sy)) {
        result.scaleY = sy;
      }
    }

    if (i3sAttrType == "feature-index") {
      result.featureIds = readUInt32Attribute(*pAttr, vertexCount);
      hasFeatureIndex = true;
    } else if (attrType == draco::GeometryAttribute::POSITION) {
      result.positions = readVec3Attribute(*pAttr, vertexCount);
    } else if (attrType == draco::GeometryAttribute::NORMAL) {
      result.normals = readVec3Attribute(*pAttr, vertexCount);
    } else if (attrType == draco::GeometryAttribute::TEX_COORD) {
      result.texCoords = readVec2Attribute(*pAttr, vertexCount);
    } else if (attrType == draco::GeometryAttribute::COLOR) {
      result.colors = readColorAttribute(*pAttr, vertexCount);
    }
    // GENERIC attributes without a known i3s-attribute-type are ignored.
  }

  // Feature count: number of distinct feature IDs.
  if (hasFeatureIndex && !result.featureIds.empty()) {
    const uint32_t maxId =
        *std::max_element(result.featureIds.begin(), result.featureIds.end());
    result.featureCount = maxId + 1;
  }

  return result;
}

} // namespace CesiumI3SContent
