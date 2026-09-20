#include "I3SCoordinateTransform.h"

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/EarthGravitationalModel1996Grid.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumUtility/Math.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace CesiumI3SContent {

namespace {

// Rotation of -90 degrees around the X axis converts from the I3S local
// convention (Y-up in the stored vertex data) to the ENU convention (Z-up).
// Matrix form of fromRotationX(-PI/2):
//  [ 1   0   0 ]
//  [ 0   0   1 ]
//  [ 0  -1   0 ]
glm::dmat3 axisFlipMatrix() {
  return glm::dmat3(
      1.0,
      0.0,
      0.0, // column 0
      0.0,
      0.0,
      -1.0, // column 1
      0.0,
      1.0,
      0.0); // column 2
}

bool needsGeoidCorrection(
    CesiumI3S::HeightModel heightModel,
    const CesiumGeospatial::EarthGravitationalModel1996Grid* pGeoidGrid) {
  return pGeoidGrid != nullptr &&
         heightModel != CesiumI3S::HeightModel::Ellipsoidal;
}

} // anonymous namespace

void I3SCoordinateTransform::transform(
    DecodedGeometry& geometry,
    const glm::dvec3& nodeCenterLonLatDegHeight,
    const glm::dquat& nodeObbRotation,
    CesiumI3S::HeightModel heightModel,
    const CesiumGeospatial::EarthGravitationalModel1996Grid* pGeoidGrid) {

  if (geometry.positions.empty()) {
    return;
  }

  const bool applyGeoid = needsGeoidCorrection(heightModel, pGeoidGrid);
  const bool hasNormals = !geometry.normals.empty();
  const bool hasUVRegion = !geometry.uvRegion.empty();
  const bool hasTexCoords = !geometry.texCoords.empty();

  // Scale factors from the decoder (1.0 for raw binary, may differ for Draco).
  const double scaleX = geometry.scaleX;
  const double scaleY = geometry.scaleY;

  // Geographic center of the node (radians).
  const double centerLon =
      CesiumUtility::Math::degreesToRadians(nodeCenterLonLatDegHeight.x);
  const double centerLat =
      CesiumUtility::Math::degreesToRadians(nodeCenterLonLatDegHeight.y);
  const double centerHeight = nodeCenterLonLatDegHeight.z;

  // ECEF position of the node center.
  const CesiumGeospatial::Cartographic centerCartographic(
      centerLon,
      centerLat,
      centerHeight);
  const glm::dvec3 ecefCenter =
      CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
          centerCartographic);

  // Build the combined parent-rotation matrix.
  // parentRotation = axisFlip * inverse(nodeObbRotation)
  const glm::dmat3 invObbMat = glm::mat3_cast(glm::inverse(nodeObbRotation));
  const glm::dmat3 parentRotation = axisFlipMatrix() * invObbMat;

  // Geoid height at the center (used to compute per-vertex deltas).
  double centerGeoidHeight = 0.0;
  if (applyGeoid) {
    centerGeoidHeight = pGeoidGrid->sampleHeight(
        CesiumGeospatial::Cartographic(centerLon, centerLat, 0.0));
  }

  const uint32_t vertexCount = static_cast<uint32_t>(geometry.positions.size());

  for (uint32_t i = 0; i < vertexCount; ++i) {
    const glm::vec3& rawPos = geometry.positions[i];

    // Step 1: Absolute geographic position.
    const double lonRad =
        centerLon + CesiumUtility::Math::degreesToRadians(
                        scaleX * static_cast<double>(rawPos.x));
    const double latRad =
        centerLat + CesiumUtility::Math::degreesToRadians(
                        scaleY * static_cast<double>(rawPos.y));
    double heightM = centerHeight + static_cast<double>(rawPos.z);

    // Step 2 (conditional): Geoid correction.
    if (applyGeoid) {
      const double vertexGeoid = pGeoidGrid->sampleHeight(
          CesiumGeospatial::Cartographic(lonRad, latRad, 0.0));
      heightM += vertexGeoid - centerGeoidHeight;
    }

    // Step 3: Geographic -> WGS84 ECEF.
    const glm::dvec3 ecef =
        CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
            CesiumGeospatial::Cartographic(lonRad, latRad, heightM));

    // Step 4: Subtract ECEF center, then Step 5: apply parentRotation.
    const glm::dvec3 local = parentRotation * (ecef - ecefCenter);

    geometry.positions[i] = glm::vec3(local);
  }

  // Transform normals (rotation only, no translation or geoid).
  if (hasNormals) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(geometry.normals.size());
         ++i) {
      const glm::dvec3 rotated =
          parentRotation * glm::dvec3(geometry.normals[i]);
      geometry.normals[i] = glm::vec3(glm::normalize(rotated));
    }
  }

  // UV region crop (Phase 5): adjust UVs to sub-region of texture atlas.
  if (hasTexCoords && hasUVRegion) {
    const uint32_t uvCount = static_cast<uint32_t>(geometry.texCoords.size());
    for (uint32_t i = 0; i < uvCount; ++i) {
      const glm::u16vec4& region = geometry.uvRegion[i];
      const float minU = static_cast<float>(region.x) / 65535.0f;
      const float minV = static_cast<float>(region.y) / 65535.0f;
      const float scaleU = static_cast<float>(region.z - region.x) / 65535.0f;
      const float scaleV = static_cast<float>(region.w - region.y) / 65535.0f;

      geometry.texCoords[i].x = geometry.texCoords[i].x * scaleU + minU;
      geometry.texCoords[i].y = geometry.texCoords[i].y * scaleV + minV;
    }
  }
}

} // namespace CesiumI3SContent
