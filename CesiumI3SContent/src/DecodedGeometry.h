#pragma once

#include <CesiumUtility/ErrorList.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

namespace CesiumI3SContent {

/**
 * @brief Decoded data from an I3S binary. All positions and normals are in the
 * node's local ENU coordinate frame after coordinate transformation. This is
 * the intermediate representation between the binary decoders and the glTF
 * assembler.
 */
struct DecodedGeometry {
  /** @brief Vertex positions in ENU local space (meters). */
  std::vector<glm::vec3> positions;
  /** @brief Per-vertex normals in ENU local space. May be empty. */
  std::vector<glm::vec3> normals;
  /** @brief Per-vertex UV texture coordinates. May be empty. */
  std::vector<glm::vec2> texCoords;
  /** @brief Per-vertex RGBA colors (sRGB, uint8 per channel). May be empty. */
  std::vector<glm::u8vec4> colors;
  /** @brief Triangle indices. Empty means non-indexed (every 3 vertices = 1
   * triangle). */
  std::vector<uint32_t> indices;
  /** @brief Per-vertex UV region [u_min, v_min, u_max, v_max] encoded as
   * uint16 in [0,65535]. Used to crop UVs from a texture atlas. May be empty.
   */
  std::vector<glm::u16vec4> uvRegion;
  /** @brief Per-vertex feature ID. May be empty. */
  std::vector<uint32_t> featureIds;
  /** @brief Number of distinct features represented in this node. */
  uint32_t featureCount = 0;
  /**
   * @brief Scale factor applied to position X (longitude) values to convert
   * them to degrees. 1.0 for raw binary; may differ for Draco-encoded
   * geometry.
   */
  double scaleX = 1.0;
  /**
   * @brief Scale factor applied to position Y (latitude) values to convert
   * them to degrees. 1.0 for raw binary; may differ for Draco-encoded
   * geometry.
   */
  double scaleY = 1.0;
  /** @brief Errors and warnings produced by the decoder. */
  CesiumUtility::ErrorList errors;
};

} // namespace CesiumI3SContent
