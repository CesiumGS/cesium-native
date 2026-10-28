#pragma once

#include "DecodedGeometry.h"

#include <CesiumGltf/Model.h>
#include <CesiumI3S/Material.h>

#include <optional>
#include <string>

namespace CesiumI3SContent {

/**
 * @brief Assembles a CesiumGltf::Model from decoded, transformed I3S node
 * geometry.
 *
 * The resulting model has a single Scene -> Node -> Mesh -> MeshPrimitive. Each
 * non-empty attribute array in `geometry` produces its own Buffer /
 * BufferView / Accessor. If featureIds are present the primitive receives an
 * EXT_mesh_features extension and the model receives a minimal
 * EXT_structural_metadata extension.
 */
struct I3SGltfAssembler {
  /**
   * @brief Build a glTF Model from decoded geometry.
   *
   * @param geometry Decoded and coordinate-transformed geometry.
   * @param pMaterialDef Optional PBR material definition. May be null.
   * @param textureUri URI of the base color texture. Empty = no texture.
   */
  static CesiumGltf::Model assemble(
      const DecodedGeometry& geometry,
      const CesiumI3S::MaterialDefinition* pMaterialDef,
      const std::string& textureUri);
};

} // namespace CesiumI3SContent
