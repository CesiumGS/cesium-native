#pragma once
#include <CesiumI3S/Library.h>

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Reference to a texture set (materialDefinitions.cmn.md). */
struct CESIUMI3S_API TextureSlotReference {
  /** @brief Index into the layer's textureSetDefinitions array. */
  uint32_t textureSetDefinitionId = 0;
  /** @brief UV coordinate set index. Default=0. */
  uint32_t texCoord = 0;
  /** @brief Multiplier applied to the texture values. Default=1. */
  double factor = 1.0;
};

/** @brief PBR metallic-roughness material model (materialDefinitions.cmn.md).
 */
struct CESIUMI3S_API PbrMetallicRoughness {
  /** @brief Base color multiplier [R, G, B, A]. Default=[1,1,1,1]. */
  std::array<double, 4> baseColorFactor = {1.0, 1.0, 1.0, 1.0};
  /** @brief Base color texture. */
  std::optional<TextureSlotReference> baseColorTexture;
  /** @brief Metalness factor [0, 1]. Default=1. */
  double metallicFactor = 1.0;
  /** @brief Roughness factor [0, 1]. Default=1. */
  double roughnessFactor = 1.0;
  /** @brief Metallic-roughness texture. */
  std::optional<TextureSlotReference> metallicRoughnessTexture;
};

/** @brief Material definition for I3S 1.7+; glTF-compatible PBR
 * (materialDefinitions.cmn.md). */
struct CESIUMI3S_API MaterialDefinition {
  /** @brief Alpha rendering mode. */
  enum class AlphaMode { Opaque, Mask, Blend };
  /** @brief Face culling mode. */
  enum class CullFace { None, Front, Back };
  /** @brief Identifies a texture slot within a material definition.
   *
   * `BaseColor` and `MetallicRoughness` live inside `pbrMetallicRoughness`;
   * the others are top-level. Use `getTexture()` to look up any slot uniformly
   * without branching on the nesting. */
  enum class TextureSemantic {
    BaseColor,
    MetallicRoughness,
    NormalMap,
    OcclusionMap,
    Emissive
  };

  /** @brief PBR metallic-roughness parameters. */
  std::optional<PbrMetallicRoughness> pbrMetallicRoughness;
  /** @brief Normal map texture. */
  std::optional<TextureSlotReference> normalTexture;
  /** @brief Ambient occlusion texture. */
  std::optional<TextureSlotReference> occlusionTexture;
  /** @brief Emissive texture. */
  std::optional<TextureSlotReference> emissiveTexture;
  /** @brief Emissive color multiplier [R, G, B]. Default=[0,0,0]. */
  std::array<double, 3> emissiveFactor = {0.0, 0.0, 0.0};
  /** @brief Alpha rendering mode. Default=Opaque. */
  AlphaMode alphaMode = AlphaMode::Opaque;
  /** @brief Alpha cutoff threshold (only for Mask mode). Default=0.25. */
  double alphaCutoff = 0.25;
  /** @brief When true, back-face culling is disabled. Default=false. */
  bool doubleSided = false;
  /** @brief Face culling override. Default=None. */
  CullFace cullFace = CullFace::None;

  /** @brief Returns a pointer to the texture for the given semantic, or
   * `nullptr` if that slot is absent. */
  const TextureSlotReference*
  getTexture(TextureSemantic semantic) const noexcept {
    switch (semantic) {
    case TextureSemantic::BaseColor:
      return (pbrMetallicRoughness && pbrMetallicRoughness->baseColorTexture)
                 ? &*pbrMetallicRoughness->baseColorTexture
                 : nullptr;
    case TextureSemantic::MetallicRoughness:
      return (pbrMetallicRoughness &&
              pbrMetallicRoughness->metallicRoughnessTexture)
                 ? &*pbrMetallicRoughness->metallicRoughnessTexture
                 : nullptr;
    case TextureSemantic::NormalMap:
      return normalTexture ? &*normalTexture : nullptr;
    case TextureSemantic::OcclusionMap:
      return occlusionTexture ? &*occlusionTexture : nullptr;
    case TextureSemantic::Emissive:
      return emissiveTexture ? &*emissiveTexture : nullptr;
    }
    return nullptr;
  }
};

/** @brief Parameters for legacy standard material type (materialParams.cmn.md,
 * deprecated I3S 1.7). */
struct CESIUMI3S_API MaterialParams {
  /** @brief Rendering mode for legacy materials. */
  enum class RenderMode { Textured, Solid, Untextured, Wireframe };

  /** @brief Transparency [0, 1]. Default=0. */
  double transparency = 0.0;
  /** @brief Reflectivity [0, 1]. Default=0. */
  double reflectivity = 0.0;
  /** @brief Shininess [0, 100]. Default=0. */
  double shininess = 0.0;
  /** @brief Ambient color [R, G, B]. Default=[0,0,0]. */
  std::vector<double> ambient = {0.0, 0.0, 0.0};
  /** @brief Diffuse color [R, G, B, A]. */
  std::vector<double> diffuse = {0.0, 0.0, 0.0, 0.0};
  /** @brief Specular color [R, G, B]. Default=[0.1,0.1,0.1]. */
  std::vector<double> specular = {0.1, 0.1, 0.1};
  /** @brief Rendering mode. */
  RenderMode renderMode = RenderMode::Textured;
  /** @brief When true, the feature casts shadows. */
  std::optional<bool> castShadows;
  /** @brief When true, the feature receives shadows. */
  std::optional<bool> receiveShadows;
  /** @brief Face culling mode. */
  std::optional<MaterialDefinition::CullFace> cullFace;
  /** @brief When true, vertex colors are used. */
  std::optional<bool> vertexColors;
  /** @brief When true, vertex UV regions are used. */
  std::optional<bool> vertexRegions;
  /** @brief When true, vertex color alpha is applied. */
  std::optional<bool> useVertexColorAlpha;
};

/** @brief Legacy material definition info (materialDefinitionInfo.cmn.md,
 * deprecated I3S 1.7). */
struct CESIUMI3S_API MaterialDefinitionInfo {
  /** @brief Legacy material type. */
  enum class Type { Standard, Water, Billboard, Leafcard, Reference };

  /** @brief Name of the material definition. */
  std::string name;
  /** @brief Legacy material type. */
  std::optional<Type> type;
  /** @brief URL to the shared resource document (deprecated). */
  std::optional<std::string> sharedResourceHref;
  /** @brief Material rendering parameters. */
  std::optional<MaterialParams> params;
};

/** @brief Legacy material definition map (sharedResource::materialDefinitions).
 */
using MaterialDefinitionMap = std::map<std::string, MaterialDefinitionInfo>;

} // namespace CesiumI3S
