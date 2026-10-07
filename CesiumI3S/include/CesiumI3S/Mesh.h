#pragma once
#include <CesiumI3S/Library.h>

#include <cstdint>
#include <optional>

namespace CesiumI3S {

/** @brief Material reference for a mesh node (meshMaterial.cmn.md). */
struct CESIUMI3S_API MeshMaterial {
  /** @brief Index in layer.materialDefinitions array. */
  uint32_t definition = 0;
  /** @brief Resource id for the material textures. Required if material
   * declares any textures. */
  std::optional<uint32_t> resource;
  /** @brief Estimated number of texels for the highest resolution base color
   * texture. */
  std::optional<uint64_t> texelCountHint;
};

/** @brief Geometry reference for a mesh node (meshGeometry.cmn.md). */
struct CESIUMI3S_API MeshGeometry {
  /** @brief Index in layer.geometryDefinitions array. */
  uint32_t definition = 0;
  /** @brief Resource locator for geometry resources. */
  uint32_t resource = 0;
  /** @brief Number of vertices in the uncompressed geometry buffer. Default=0.
   */
  uint64_t vertexCount = 0;
  /** @brief Number of features for this mesh. Default=0. */
  uint64_t featureCount = 0;
};

/** @brief Attribute reference for a mesh node (meshAttribute.cmn.md). */
struct CESIUMI3S_API MeshAttribute {
  /** @brief Resource identifier for the attribute resources of this mesh. */
  uint32_t resource = 0;
};

/** @brief Mesh geometry for a node (mesh.cmn.md). */
struct CESIUMI3S_API Mesh {
  /** @brief The geometry definition. */
  MeshGeometry geometry;
  /** @brief The material definition. */
  std::optional<MeshMaterial> material;
  /** @brief The attribute set definition. */
  std::optional<MeshAttribute> attribute;
};

} // namespace CesiumI3S
