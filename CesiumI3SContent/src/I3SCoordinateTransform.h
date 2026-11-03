#pragma once

#include "DecodedGeometry.h"

#include <CesiumGeospatial/EarthGravitationalModel1996Grid.h>
#include <CesiumI3S/SpatialReference.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace CesiumI3SContent {

/**
 * @brief Transforms decoded I3S vertex positions from the raw geographic-offset
 * space into a node-local ENU Cartesian frame, and optionally crops UV
 * coordinates using UV region data.
 *
 * After this call, `geometry.positions` and `geometry.normals` are in a
 * right-handed ENU coordinate frame centred on the node's ECEF position,
 * with axes oriented via the node's OBB quaternion. The units are metres.
 * UV coordinates are adjusted to the sub-region of the texture atlas when
 * `geometry.uvRegion` is non-empty.
 */
struct I3SCoordinateTransform {
  /**
   * @brief Transform positions and normals in-place and crop UV coordinates.
   *
   * @param geometry Decoded geometry to transform in-place.
   * @param nodeCenterLonLatDegHeight Node OBB / MBS centre
   *   (longitude degrees, latitude degrees, height metres).
   * @param nodeObbRotation Node OBB orientation quaternion.
   * @param heightModel Height model reported by the layer (controls whether
   *   geoid correction is applied).
   * @param pGeoidGrid EGM96 grid for geoid correction. Ignored when null or
   *   when heightModel is Ellipsoidal.
   */
  static void transform(
      DecodedGeometry& geometry,
      const glm::dvec3& nodeCenterLonLatDegHeight,
      const glm::dquat& nodeObbRotation,
      CesiumI3S::HeightModel heightModel,
      const CesiumGeospatial::EarthGravitationalModel1996Grid* pGeoidGrid);
};

} // namespace CesiumI3SContent
