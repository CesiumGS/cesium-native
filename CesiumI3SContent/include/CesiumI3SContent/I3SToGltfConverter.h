#pragma once

#include <CesiumGeospatial/EarthGravitationalModel1996Grid.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/GeometrySchema.h>
#include <CesiumI3S/Material.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3SContent/I3SConverterResult.h>
#include <CesiumI3SContent/Library.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>

namespace CesiumI3SContent {

/**
 * @brief Input for a single I3S node geometry conversion.
 *
 * Exactly one of `pGeometryBuffer` (modern 1.7+) or `pDefaultSchema` (legacy)
 * must be non-null.
 */
struct CESIUMI3SCONTENT_API I3SToGltfConverterInput {
  /** @brief Raw geometry buffer bytes (binary or Draco). */
  std::span<const std::byte> geometryBytes;

  /**
   * @brief Modern (1.7+) geometry buffer descriptor. Mutually exclusive with
   * `pDefaultSchema`. Pass null for legacy layers.
   */
  const CesiumI3S::GeometryBuffer* pGeometryBuffer = nullptr;

  /**
   * @brief Legacy geometry schema. Mutually exclusive with `pGeometryBuffer`.
   * Pass null for modern layers.
   */
  const CesiumI3S::GeometrySchema* pDefaultSchema = nullptr;

  /**
   * @brief Node OBB / MBS centre (longitude degrees, latitude degrees, height
   * metres above the reference ellipsoid).
   */
  glm::dvec3 nodeCenterLonLatDegHeight{0.0, 0.0, 0.0};

  /**
   * @brief Node OBB orientation quaternion (identity = ENU-aligned).
   */
  glm::dquat nodeObbRotation{1.0, 0.0, 0.0, 0.0};

  /**
   * @brief PBR material definition for the node. May be null (produces a
   * default material when a texture is available).
   */
  const CesiumI3S::MaterialDefinition* pMaterialDef = nullptr;

  /**
   * @brief Full URI of the base color texture resource. Empty = no texture.
   */
  std::string textureUri;

  /**
   * @brief EGM96 geoid height grid used to apply a geoid correction to vertex
   * heights. When null the geoid correction step is skipped entirely.
   */
  const CesiumGeospatial::EarthGravitationalModel1996Grid* pGeoidGrid = nullptr;

  /**
   * @brief Horizontal spatial reference of the layer. Only WGS84 geographic
   * (WKID 4326) is supported; other CRS values produce a conversion error.
   */
  CesiumI3S::SpatialReference spatialReference;

  /**
   * @brief Height model information from the layer. Controls whether geoid
   * correction is applied and validates the vertical CRS.
   */
  std::optional<CesiumI3S::HeightModelInfo> heightModelInfo;
};

/**
 * @brief Converts an I3S node geometry binary into a CesiumGltf::Model.
 *
 * Supports CMN (3DObject, IntegratedMesh) and PSL (Point) layer types, with
 * both legacy (< 1.7) and modern (>= 1.7) geometry schemas, raw binary and
 * Draco-compressed geometry, and optional EGM96 geoid correction.
 */
struct CESIUMI3SCONTENT_API I3SToGltfConverter {
  /**
   * @brief Convert an I3S node geometry binary to a glTF Model.
   *
   * @param input Conversion parameters including geometry bytes, schema,
   *   node spatial context, material, and optional geoid grid.
   * @return Conversion result containing the glTF model and any errors or
   *   warnings encountered during conversion.
   */
  static I3SConverterResult convert(const I3SToGltfConverterInput& input);
};

} // namespace CesiumI3SContent
