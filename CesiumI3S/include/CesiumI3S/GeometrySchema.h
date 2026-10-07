#pragma once
#include <CesiumI3S/AttributeStorage.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/Library.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Accessor into a vertex or face array buffer
 * (geometryAttribute.cmn.md). */
struct CESIUMI3S_API GeometryAttribute {
  /** @brief Starting byte position. Only used with arrayBufferView geometry. */
  std::optional<uint64_t> byteOffset;
  /** @brief The element type. */
  DataType valueType = DataType::Float32;
  /** @brief Number of values needed to make one element (e.g. 3 for xyz
   * position). */
  uint32_t valuesPerElement = 1;
};

/** @brief Per-vertex attribute declarations in the geometry
 * (vertexAttribute.cmn.md). */
struct CESIUMI3S_API VertexAttribute {
  /** @brief Vertex x,y,z positions (Float32[3*vertexCount]). */
  GeometryAttribute position;
  /** @brief Normals x,y,z vectors (Float32[3*vertexCount]). */
  GeometryAttribute normal;
  /** @brief Texture coordinates (Float32[2*vertexCount]). */
  GeometryAttribute uv0;
  /** @brief RGBA colors (UInt8[4*vertexCount]). */
  GeometryAttribute color;
  /** @brief UV region for repeated textures in a texture atlas
   * (UInt16[4*vertexCount]). */
  std::optional<GeometryAttribute> region;
};

/** @brief Common geometry buffer layout shared across all nodes in a store
 * (defaultGeometrySchema.cmn.md). */
struct CESIUMI3S_API GeometrySchema {
  /** @brief How vertex attributes are packed in the geometry binary. */
  enum class BufferLayout { PerAttributeArray, Indexed };
  /** @brief Vertex attribute field name, used in ordering arrays. */
  enum class AttributeField { Position, Normal, Uv0, Color, Region };

  /** @brief Geometry type; always Triangle for CMN layers. */
  std::optional<GeometryTopology> geometryType;
  /** @brief Buffer packing layout. */
  BufferLayout topology = BufferLayout::PerAttributeArray;
  /** @brief Header fields in geometry resources that precede vertex data. */
  std::vector<HeaderAttribute> header;
  /** @brief Ordering of the vertex attribute arrays in the geometry binary. */
  std::vector<AttributeField> ordering;
  /** @brief Declaration of per-vertex attributes (position, normals, texture
   * coordinates). */
  VertexAttribute vertexAttributes;
  /** @brief Declaration of face index attributes (for Indexed layout). */
  std::optional<VertexAttribute> faces;
  /** @brief Order of the keys in featureAttributes. */
  std::vector<std::string> featureAttributeOrder;
  /** @brief Declaration of per-feature attributes (feature ID, face range). */
  FeatureAttribute featureAttributes;
};

} // namespace CesiumI3S
