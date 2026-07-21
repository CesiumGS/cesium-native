#pragma once
#include <CesiumI3S/Library.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Compressed data. Cannot be combined with any other
 * vertex attributes (compression.cmn.md). */
struct CESIUMI3S_API Compression {
  /** @brief Compression encoding scheme. Draco is used for CMN mesh geometry;
   * Lepcc is used for PCSL point-cloud geometry (LEPCC-XYZ). */
  enum class Encoding { Draco, Lepcc };
  /** @brief Vertex attributes included in the compressed buffer. */
  enum class Attribute { Position, Normal, Uv0, Color, UvRegion, FeatureIndex };

  /** @brief Encoding scheme. */
  Encoding encoding = Encoding::Draco;
  /** @brief Attributes included in the compressed buffer. */
  std::vector<Attribute> attributes;
};

/** @brief Mesh geometry description for a single geometry buffer
 * (geometryBuffer.cmn.md). */
struct CESIUMI3S_API GeometryBuffer {
  /** @brief Descriptor for one vertex attribute slot in the buffer. */
  struct Attribute {
    /** @brief How many instances of this attribute are stored per element. */
    enum class Binding {
      /** @brief One value per vertex. The default for most attributes. */
      PerVertex,
      /** @brief One value per feature. Used for `featureId` and `faceRange`. */
      PerFeature
    };

    /** @brief Number of components per element (e.g. 3 for xyz). 0 = absent. */
    uint32_t component = 0;
    /** @brief Binding granularity. Default is `PerVertex`. */
    Binding binding = Binding::PerVertex;
    /** @brief Returns true when this attribute is included in the buffer. */
    bool isPresent() const noexcept { return component > 0; }
  };

  /** @brief Feature ID attribute. Extends `Attribute` with a storage type,
   * since the spec allows `UInt16`, `UInt32`, or `UInt64` encoding. */
  struct FeatureIdAttribute : Attribute {
    /** @brief Integer width used to store each feature ID. */
    enum class Type { UInt16, UInt32, UInt64 };

    /** @brief Feature ID storage type. Default is `UInt32`. */
    Type type = Type::UInt32;
  };

  /** @brief Bytes to skip at the start of the binary buffer. Default=0. */
  uint64_t offset = 0;
  /** @brief Vertex positions relative to OBB center (Float32, component=3). */
  Attribute position;
  /** @brief Face/vertex normals (Float32, component=3). */
  Attribute normal;
  /** @brief First set of UV coordinates, textured meshes only (Float32,
   * component=2). */
  Attribute uv0;
  /** @brief Vertex colors (UInt8, component=1,3, or 4). */
  Attribute color;
  /** @brief UV regions for texture atlases (UInt16, component=4). */
  Attribute uvRegion;
  /** @brief Feature IDs. Type and component are set from the spec's
   * `geometryFeatureID` descriptor. */
  FeatureIdAttribute featureId;
  /** @brief Face ranges per feature (UInt32, component=2). */
  Attribute faceRange;
  /** @brief Compressed attributes. Present means the whole buffer is
   * compressed. */
  std::optional<Compression> compression;

  /** @brief Returns true when the buffer is Draco-compressed. */
  bool isDraco() const noexcept {
    return compression.has_value() &&
           compression->encoding == Compression::Encoding::Draco;
  }

  /** @brief Returns true when the buffer is LEPCC-compressed. */
  bool isLepcc() const noexcept {
    return compression.has_value() &&
           compression->encoding == Compression::Encoding::Lepcc;
  };
};

/** @brief Topology type for a geometry definition (geometryDefinition.cmn.md).
 */
enum class GeometryTopology {
  /** @brief Used for CMN (3DObject, IntegratedMesh) profiles. */
  Triangle,
  /** @brief Used for PSL (Point) profile. */
  Point
};

/** @brief Geometry representation(s) for a class of meshes
 * (geometryDefinition.cmn.md). */
struct CESIUMI3S_API GeometryDefinition {
  /** @brief Topology type of the mesh. */
  std::optional<GeometryTopology> topology;
  /** @brief Geometry buffer representations. Uncompressed buffer must be first.
   */
  std::vector<GeometryBuffer> geometryBuffers;
};

/** @brief Geometry entry embedded in a FeatureData payload
 * (geometry.cmn.md). */
struct CESIUMI3S_API Geometry {
  /** @brief How the geometry data is stored or referenced. */
  enum class Storage {
    ArrayBufferView,
    GeometryReference,
    SharedResourceReference,
    Embedded
  };

  /** @brief Unique ID of the geometry in this store. */
  uint64_t id = 0;
  /** @brief How the geometry data is addressed. */
  std::optional<Storage> type;
  /** @brief Local-to-world transformation matrix (column-major, 4×4). */
  std::optional<std::array<double, 16>> transformation;
  /** @brief Geometry parameters; raw JSON (spec type is polymorphic oneOf
   * geometryReferenceParams / vestedGeometryParams / singleComponentParams). */
  std::optional<std::string> params;
};

} // namespace CesiumI3S
