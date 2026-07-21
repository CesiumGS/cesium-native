#include "I3SBinaryDecoder.h"

#include <CesiumI3S/AttributeStorage.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumUtility/ErrorList.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <cstring>
#include <span>
#include <string>

namespace CesiumI3SContent {

namespace {

uint32_t readU32LE(std::span<const std::byte> data, size_t offset) {
  uint32_t v = 0;
  std::memcpy(&v, data.data() + offset, sizeof(v));
  // I3S binary is always little-endian; no swap needed on LE hosts.
  // For correctness on big-endian hosts we'd need to byte-swap here,
  // but cesium-native only targets LE platforms.
  return v;
}

float readF32LE(std::span<const std::byte> data, size_t offset) {
  float v = 0.0f;
  std::memcpy(&v, data.data() + offset, sizeof(v));
  return v;
}

uint16_t readU16LE(std::span<const std::byte> data, size_t offset) {
  uint16_t v = 0;
  std::memcpy(&v, data.data() + offset, sizeof(v));
  return v;
}

// Attribute readers — each returns the new cursor position

size_t readPositions(
    std::span<const std::byte> data,
    size_t cursor,
    uint32_t vertexCount,
    std::vector<glm::vec3>& out) {
  out.resize(vertexCount);
  for (uint32_t i = 0; i < vertexCount; ++i) {
    out[i].x = readF32LE(data, cursor);
    cursor += 4;
    out[i].y = readF32LE(data, cursor);
    cursor += 4;
    out[i].z = readF32LE(data, cursor);
    cursor += 4;
  }
  return cursor;
}

size_t readNormals(
    std::span<const std::byte> data,
    size_t cursor,
    uint32_t vertexCount,
    std::vector<glm::vec3>& out) {
  out.resize(vertexCount);
  for (uint32_t i = 0; i < vertexCount; ++i) {
    out[i].x = readF32LE(data, cursor);
    cursor += 4;
    out[i].y = readF32LE(data, cursor);
    cursor += 4;
    out[i].z = readF32LE(data, cursor);
    cursor += 4;
  }
  return cursor;
}

size_t readTexCoords(
    std::span<const std::byte> data,
    size_t cursor,
    uint32_t vertexCount,
    std::vector<glm::vec2>& out) {
  out.resize(vertexCount);
  for (uint32_t i = 0; i < vertexCount; ++i) {
    out[i].x = readF32LE(data, cursor);
    cursor += 4;
    out[i].y = readF32LE(data, cursor);
    cursor += 4;
  }
  return cursor;
}

size_t readColors(
    std::span<const std::byte> data,
    size_t cursor,
    uint32_t vertexCount,
    std::vector<glm::u8vec4>& out) {
  out.resize(vertexCount);
  for (uint32_t i = 0; i < vertexCount; ++i) {
    out[i].x = static_cast<uint8_t>(data[cursor]);
    cursor += 1;
    out[i].y = static_cast<uint8_t>(data[cursor]);
    cursor += 1;
    out[i].z = static_cast<uint8_t>(data[cursor]);
    cursor += 1;
    out[i].w = static_cast<uint8_t>(data[cursor]);
    cursor += 1;
  }
  return cursor;
}

size_t readUVRegion(
    std::span<const std::byte> data,
    size_t cursor,
    uint32_t vertexCount,
    std::vector<glm::u16vec4>& out) {
  out.resize(vertexCount);
  for (uint32_t i = 0; i < vertexCount; ++i) {
    out[i].x = readU16LE(data, cursor);
    cursor += 2;
    out[i].y = readU16LE(data, cursor);
    cursor += 2;
    out[i].z = readU16LE(data, cursor);
    cursor += 2;
    out[i].w = readU16LE(data, cursor);
    cursor += 2;
  }
  return cursor;
}

/**
 * @brief Expand per-feature faceRange into a per-vertex feature ID array.
 *
 * faceRange layout: [startFace, endFace (inclusive)] per feature.
 * For non-indexed geometry, face i corresponds to vertices [3i, 3i+1, 3i+2].
 */
void expandFaceRange(
    std::span<const std::byte> data,
    size_t cursor,
    uint32_t featureCount,
    uint32_t vertexCount,
    std::vector<uint32_t>& featureIds) {
  featureIds.resize(vertexCount, 0);
  for (uint32_t f = 0; f < featureCount; ++f) {
    const uint32_t startFace = readU32LE(data, cursor);
    cursor += 4;
    const uint32_t endFace = readU32LE(data, cursor);
    cursor += 4;
    for (uint32_t face = startFace; face <= endFace; ++face) {
      const uint32_t base = face * 3;
      if (base + 2 < vertexCount) {
        featureIds[base] = f;
        featureIds[base + 1] = f;
        featureIds[base + 2] = f;
      }
    }
  }
}

size_t headerAttributeByteSize(CesiumI3S::DataType type) {
  switch (type) {
  case CesiumI3S::DataType::UInt8:
    return 1;
  case CesiumI3S::DataType::UInt16:
  case CesiumI3S::DataType::Int16:
    return 2;
  case CesiumI3S::DataType::UInt32:
  case CesiumI3S::DataType::Int32:
  case CesiumI3S::DataType::Float32:
    return 4;
  case CesiumI3S::DataType::UInt64:
  case CesiumI3S::DataType::Int64:
  case CesiumI3S::DataType::Float64:
    return 8;
  default:
    return 4;
  }
}

} // anonymous namespace

DecodedGeometry I3SBinaryDecoder::decodeLegacy(
    std::span<const std::byte> data,
    const CesiumI3S::GeometrySchema& schema) {
  DecodedGeometry result;

  // --- Read header ---
  size_t cursor = 0;
  uint32_t vertexCount = 0;
  uint32_t featureCount = 0;

  for (const auto& hdr : schema.header) {
    if (cursor + headerAttributeByteSize(hdr.type) > data.size()) {
      result.errors.emplaceError("I3S binary header is truncated");
      return result;
    }
    if (hdr.property == "vertexCount") {
      vertexCount = readU32LE(data, cursor);
    } else if (hdr.property == "featureCount") {
      featureCount = readU32LE(data, cursor);
    }
    cursor += headerAttributeByteSize(hdr.type);
  }

  if (vertexCount == 0) {
    // Empty geometry is valid; nothing to decode.
    return result;
  }

  result.featureCount = featureCount;

  // --- Read vertex attributes in schema ordering ---
  for (const CesiumI3S::GeometrySchema::AttributeField field :
       schema.ordering) {
    switch (field) {
    case CesiumI3S::GeometrySchema::AttributeField::Position:
      if (cursor + 12ull * vertexCount > data.size()) {
        result.errors.emplaceError(
            "I3S binary buffer truncated reading positions");
        return result;
      }
      cursor = readPositions(data, cursor, vertexCount, result.positions);
      break;
    case CesiumI3S::GeometrySchema::AttributeField::Normal:
      if (cursor + 12ull * vertexCount > data.size()) {
        result.errors.emplaceError(
            "I3S binary buffer truncated reading normals");
        return result;
      }
      cursor = readNormals(data, cursor, vertexCount, result.normals);
      break;
    case CesiumI3S::GeometrySchema::AttributeField::Uv0:
      if (cursor + 8ull * vertexCount > data.size()) {
        result.errors.emplaceError(
            "I3S binary buffer truncated reading UV coordinates");
        return result;
      }
      cursor = readTexCoords(data, cursor, vertexCount, result.texCoords);
      break;
    case CesiumI3S::GeometrySchema::AttributeField::Color:
      if (cursor + 4ull * vertexCount > data.size()) {
        result.errors.emplaceError(
            "I3S binary buffer truncated reading colors");
        return result;
      }
      cursor = readColors(data, cursor, vertexCount, result.colors);
      break;
    case CesiumI3S::GeometrySchema::AttributeField::Region:
      if (cursor + 8ull * vertexCount > data.size()) {
        result.errors.emplaceError(
            "I3S binary buffer truncated reading UV regions");
        return result;
      }
      cursor = readUVRegion(data, cursor, vertexCount, result.uvRegion);
      break;
    }
    // All valid VertexAttributeField values are handled above.
  }

  // Read feature attributes
  std::vector<uint32_t> faceRangeData;
  bool hasFaceRange = false;

  for (const auto& attr : schema.featureAttributeOrder) {
    if (attr == "featureId" || attr == "id") {
      // uint64 per feature — not used, just skip
      cursor += 8ull * featureCount;
    } else if (attr == "faceRange") {
      if (cursor + 8ull * featureCount > data.size()) {
        result.errors.emplaceWarning(
            "I3S binary buffer truncated reading face ranges");
        break;
      }
      expandFaceRange(
          data,
          cursor,
          featureCount,
          vertexCount,
          result.featureIds);
      hasFaceRange = true;
      cursor += 8ull * featureCount;
    }
  }

  (void)hasFaceRange;
  return result;
}

DecodedGeometry I3SBinaryDecoder::decodeModern(
    std::span<const std::byte> data,
    const CesiumI3S::GeometryBuffer& geometryBuffer) {
  DecodedGeometry result;

  // Guard: Draco magic "DRACO\0"
  if (data.size() >= 5 && static_cast<char>(data[0]) == 'D' &&
      static_cast<char>(data[1]) == 'R' && static_cast<char>(data[2]) == 'A' &&
      static_cast<char>(data[3]) == 'C' && static_cast<char>(data[4]) == 'O') {
    result.errors.emplaceError(
        "Buffer contains Draco-encoded geometry; use I3SDracoDecoder instead");
    return result;
  }

  // Guard: LEPCC encoding
  if (geometryBuffer.compression.has_value()) {
    const auto& ca = *geometryBuffer.compression;
    result.errors.emplaceError(
        "Unsupported geometry buffer encoding: " +
        std::to_string(static_cast<int>(ca.encoding)) +
        ". Use the appropriate decoder.");
    return result;
  }

  // Determine vertex and feature counts
  // Modern uncompressed buffers follow the same convention as legacy:
  // [vertexCount: uint32][featureCount: uint32] at offset 0, then
  // attribute data starting at geometryBuffer.offset (typically 8).
  if (data.size() < 8) {
    result.errors.emplaceError("I3S modern binary buffer is too small");
    return result;
  }

  const uint32_t vertexCount = readU32LE(data, 0);
  const uint32_t featureCount = readU32LE(data, 4);

  if (vertexCount == 0) {
    return result;
  }

  result.featureCount = featureCount;

  size_t cursor = static_cast<size_t>(geometryBuffer.offset);
  if (cursor < 8) {
    // offset=0 means no separate header; attribute data starts at byte 0 but
    // we already consumed the first 8 bytes interpreting them as the header.
    // If offset is genuinely 0, reset cursor to 0 (attribute data overlaps
    // the header region which would be unusual but follow the descriptor).
    cursor = 0;
  }

  // Read vertex attributes in the fixed descriptor order
  if (geometryBuffer.position.isPresent()) {
    if (cursor + 12ull * vertexCount > data.size()) {
      result.errors.emplaceError(
          "I3S modern binary buffer truncated reading positions");
      return result;
    }
    cursor = readPositions(data, cursor, vertexCount, result.positions);
  }

  if (geometryBuffer.normal.isPresent()) {
    if (cursor + 12ull * vertexCount > data.size()) {
      result.errors.emplaceError(
          "I3S modern binary buffer truncated reading normals");
      return result;
    }
    cursor = readNormals(data, cursor, vertexCount, result.normals);
  }

  if (geometryBuffer.uv0.isPresent()) {
    if (cursor + 8ull * vertexCount > data.size()) {
      result.errors.emplaceError(
          "I3S modern binary buffer truncated reading UV coordinates");
      return result;
    }
    cursor = readTexCoords(data, cursor, vertexCount, result.texCoords);
  }

  if (geometryBuffer.color.isPresent()) {
    const uint32_t comp = geometryBuffer.color.component;
    const uint64_t colorBytes = static_cast<uint64_t>(comp) * vertexCount;
    if (cursor + colorBytes > data.size()) {
      result.errors.emplaceError(
          "I3S modern binary buffer truncated reading colors");
      return result;
    }
    // Always store as RGBA; pad missing channels with 255.
    result.colors.resize(vertexCount, {255, 255, 255, 255});
    for (uint32_t i = 0; i < vertexCount; ++i) {
      if (comp > 0)
        result.colors[i].x = static_cast<uint8_t>(data[cursor + i * comp]);
      if (comp > 1)
        result.colors[i].y = static_cast<uint8_t>(data[cursor + i * comp + 1]);
      if (comp > 2)
        result.colors[i].z = static_cast<uint8_t>(data[cursor + i * comp + 2]);
      if (comp > 3)
        result.colors[i].w = static_cast<uint8_t>(data[cursor + i * comp + 3]);
    }
    cursor += colorBytes;
  }

  if (geometryBuffer.uvRegion.isPresent()) {
    if (cursor + 8ull * vertexCount > data.size()) {
      result.errors.emplaceError(
          "I3S modern binary buffer truncated reading UV regions");
      return result;
    }
    cursor = readUVRegion(data, cursor, vertexCount, result.uvRegion);
  }

  if (geometryBuffer.featureId.isPresent()) {
    // Per-feature uint64 feature ID — not used directly; skip.
    cursor += 8ull * featureCount;
  }

  if (geometryBuffer.faceRange.isPresent()) {
    if (cursor + 8ull * featureCount > data.size()) {
      result.errors.emplaceWarning(
          "I3S modern binary buffer truncated reading face ranges");
      return result;
    }
    expandFaceRange(data, cursor, featureCount, vertexCount, result.featureIds);
    cursor += 8ull * featureCount;
  }

  (void)cursor;
  return result;
}

} // namespace CesiumI3SContent
