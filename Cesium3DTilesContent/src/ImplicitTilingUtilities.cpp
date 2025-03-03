#include <Cesium3DTiles/BoundingVolume.h>
#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesContent/TileBoundingVolumes.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Uri.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_double.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <libmorton/morton.h>

#include <cstdint>
#include <optional>
#include <string>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesContent {

std::string ImplicitTilingUtilities::resolveUrl(
    const std::string& baseUrl,
    const std::string& urlTemplate,
    const QuadtreeTileID& quadtreeID) {
  std::string url = CesiumUtility::Uri::substituteTemplateParameters(
      urlTemplate,
      [&quadtreeID](const std::string& placeholder) {
        if (placeholder == "level") {
          return std::to_string(quadtreeID.level);
        }
        if (placeholder == "x") {
          return std::to_string(quadtreeID.x);
        }
        if (placeholder == "y") {
          return std::to_string(quadtreeID.y);
        }

        return placeholder;
      });

  return CesiumUtility::Uri::resolve(baseUrl, url);
}

std::string ImplicitTilingUtilities::resolveUrl(
    const std::string& baseUrl,
    const std::string& urlTemplate,
    const OctreeTileID& octreeID) {
  std::string url = CesiumUtility::Uri::substituteTemplateParameters(
      urlTemplate,
      [&octreeID](const std::string& placeholder) {
        if (placeholder == "level") {
          return std::to_string(octreeID.level);
        }
        if (placeholder == "x") {
          return std::to_string(octreeID.x);
        }
        if (placeholder == "y") {
          return std::to_string(octreeID.y);
        }
        if (placeholder == "z") {
          return std::to_string(octreeID.z);
        }

        return placeholder;
      });

  return CesiumUtility::Uri::resolve(baseUrl, url);
}

uint64_t ImplicitTilingUtilities::computeMortonIndex(
    const CesiumGeometry::QuadtreeTileID& tileID) {
  return libmorton::morton2D_64_encode(tileID.x, tileID.y);
}

uint64_t ImplicitTilingUtilities::computeMortonIndex(
    const CesiumGeometry::OctreeTileID& tileID) {
  return libmorton::morton3D_64_encode(tileID.x, tileID.y, tileID.z);
}

uint64_t ImplicitTilingUtilities::computeRelativeMortonIndex(
    const QuadtreeTileID& subtreeID,
    const QuadtreeTileID& tileID) {
  return computeMortonIndex(absoluteTileIDToRelative(subtreeID, tileID));
}

uint64_t ImplicitTilingUtilities::computeRelativeMortonIndex(
    const OctreeTileID& subtreeID,
    const OctreeTileID& tileID) {
  return computeMortonIndex(absoluteTileIDToRelative(subtreeID, tileID));
}

CesiumGeometry::QuadtreeTileID ImplicitTilingUtilities::getSubtreeRootID(
    uint32_t subtreeLevels,
    const CesiumGeometry::QuadtreeTileID& tileID) noexcept {
  uint32_t subtreeLevel = tileID.level / subtreeLevels;
  uint32_t levelsLeft = tileID.level % subtreeLevels;
  return QuadtreeTileID(
      subtreeLevel * subtreeLevels,
      tileID.x >> levelsLeft,
      tileID.y >> levelsLeft);
}

CesiumGeometry::OctreeTileID ImplicitTilingUtilities::getSubtreeRootID(
    uint32_t subtreeLevels,
    const CesiumGeometry::OctreeTileID& tileID) noexcept {
  uint32_t subtreeLevel = tileID.level / subtreeLevels;
  uint32_t levelsLeft = tileID.level % subtreeLevels;
  return OctreeTileID(
      subtreeLevel * subtreeLevels,
      tileID.x >> levelsLeft,
      tileID.y >> levelsLeft,
      tileID.z >> levelsLeft);
}

QuadtreeTileID ImplicitTilingUtilities::absoluteTileIDToRelative(
    const QuadtreeTileID& rootID,
    const QuadtreeTileID& tileID) noexcept {
  uint32_t relativeTileLevel = tileID.level - rootID.level;
  return QuadtreeTileID(
      relativeTileLevel,
      tileID.x - (rootID.x << relativeTileLevel),
      tileID.y - (rootID.y << relativeTileLevel));
}

OctreeTileID ImplicitTilingUtilities::absoluteTileIDToRelative(
    const OctreeTileID& rootID,
    const OctreeTileID& tileID) noexcept {
  uint32_t relativeTileLevel = tileID.level - rootID.level;
  return OctreeTileID(
      relativeTileLevel,
      tileID.x - (rootID.x << relativeTileLevel),
      tileID.y - (rootID.y << relativeTileLevel),
      tileID.z - (rootID.z << relativeTileLevel));
}

namespace {
template <typename T>
Cesium3DTiles::BoundingVolume computeBoundingVolumeInternal(
    const Cesium3DTiles::BoundingVolume& rootBoundingVolume,
    const T& tileID,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  Cesium3DTiles::BoundingVolume result;

  std::optional<OrientedBoundingBox> maybeBox =
      TileBoundingVolumes::getOrientedBoundingBox(rootBoundingVolume);
  if (maybeBox) {
    OrientedBoundingBox obb =
        ImplicitTilingUtilities::computeBoundingVolume(*maybeBox, tileID);
    TileBoundingVolumes::setOrientedBoundingBox(result, obb);
  }

  std::optional<BoundingRegion> maybeRegion =
      TileBoundingVolumes::getBoundingRegion(rootBoundingVolume, ellipsoid);
  if (maybeRegion) {
    BoundingRegion region = ImplicitTilingUtilities::computeBoundingVolume(
        *maybeRegion,
        tileID,
        ellipsoid);
    TileBoundingVolumes::setBoundingRegion(result, region);
  }

  std::optional<S2CellBoundingVolume> maybeS2 =
      TileBoundingVolumes::getS2CellBoundingVolume(
          rootBoundingVolume,
          ellipsoid);
  if (maybeS2) {
    S2CellBoundingVolume s2 = ImplicitTilingUtilities::computeBoundingVolume(
        *maybeS2,
        tileID,
        ellipsoid);
    TileBoundingVolumes::setS2CellBoundingVolume(result, s2);
  }

  std::optional<BoundingCylinderRegion> maybeCylinder =
      TileBoundingVolumes::getBoundingCylinderRegion(rootBoundingVolume);
  if (maybeCylinder) {
    BoundingCylinderRegion cylinder =
        ImplicitTilingUtilities::computeBoundingVolume(*maybeCylinder, tileID);
    TileBoundingVolumes::setBoundingCylinderRegion(result, cylinder);
  }

  return result;
}
} // namespace

Cesium3DTiles::BoundingVolume ImplicitTilingUtilities::computeBoundingVolume(
    const Cesium3DTiles::BoundingVolume& rootBoundingVolume,
    const CesiumGeometry::QuadtreeTileID& tileID,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  return computeBoundingVolumeInternal(rootBoundingVolume, tileID, ellipsoid);
}

Cesium3DTiles::BoundingVolume ImplicitTilingUtilities::computeBoundingVolume(
    const Cesium3DTiles::BoundingVolume& rootBoundingVolume,
    const CesiumGeometry::OctreeTileID& tileID,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  return computeBoundingVolumeInternal(rootBoundingVolume, tileID, ellipsoid);
}

namespace {

CesiumGeospatial::GlobeRectangle subdivideRectangle(
    const CesiumGeospatial::GlobeRectangle& rootRectangle,
    const CesiumGeometry::QuadtreeTileID& tileID,
    double denominator) {
  double latSize =
      (rootRectangle.getNorth() - rootRectangle.getSouth()) / denominator;
  double longSize =
      (rootRectangle.getEast() - rootRectangle.getWest()) / denominator;

  double childWest = rootRectangle.getWest() + longSize * tileID.x;
  double childEast = rootRectangle.getWest() + longSize * (tileID.x + 1);

  double childSouth = rootRectangle.getSouth() + latSize * tileID.y;
  double childNorth = rootRectangle.getSouth() + latSize * (tileID.y + 1);

  return CesiumGeospatial::GlobeRectangle(
      childWest,
      childSouth,
      childEast,
      childNorth);
}

} // namespace

CesiumGeospatial::BoundingRegion ImplicitTilingUtilities::computeBoundingVolume(
    const CesiumGeospatial::BoundingRegion& rootBoundingVolume,
    const CesiumGeometry::QuadtreeTileID& tileID,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  double denominator = computeLevelDenominator(tileID.level);
  return CesiumGeospatial::BoundingRegion{
      subdivideRectangle(
          rootBoundingVolume.getRectangle(),
          tileID,
          denominator),
      rootBoundingVolume.getMinimumHeight(),
      rootBoundingVolume.getMaximumHeight(),
      ellipsoid};
}

CesiumGeospatial::BoundingRegion ImplicitTilingUtilities::computeBoundingVolume(
    const CesiumGeospatial::BoundingRegion& rootBoundingVolume,
    const CesiumGeometry::OctreeTileID& tileID,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  double denominator = computeLevelDenominator(tileID.level);
  double heightSize = (rootBoundingVolume.getMaximumHeight() -
                       rootBoundingVolume.getMinimumHeight()) /
                      denominator;

  double childMinHeight =
      rootBoundingVolume.getMinimumHeight() + heightSize * tileID.z;
  double childMaxHeight =
      rootBoundingVolume.getMinimumHeight() + heightSize * (tileID.z + 1);

  return CesiumGeospatial::BoundingRegion{
      subdivideRectangle(
          rootBoundingVolume.getRectangle(),
          QuadtreeTileID(tileID.level, tileID.x, tileID.y),
          denominator),
      childMinHeight,
      childMaxHeight,
      ellipsoid};
}

CesiumGeometry::OrientedBoundingBox
ImplicitTilingUtilities::computeBoundingVolume(
    const CesiumGeometry::OrientedBoundingBox& rootBoundingVolume,
    const CesiumGeometry::QuadtreeTileID& tileID) noexcept {
  const glm::dmat3& halfAxes = rootBoundingVolume.getHalfAxes();
  const glm::dvec3& center = rootBoundingVolume.getCenter();

  double denominator = computeLevelDenominator(tileID.level);
  glm::dvec3 min = center - halfAxes[0] - halfAxes[1] - halfAxes[2];

  glm::dvec3 xDim = halfAxes[0] * 2.0 / denominator;
  glm::dvec3 yDim = halfAxes[1] * 2.0 / denominator;
  glm::dvec3 childMin = min + xDim * double(tileID.x) + yDim * double(tileID.y);
  glm::dvec3 childMax = min + xDim * double(tileID.x + 1) +
                        yDim * double(tileID.y + 1) + halfAxes[2] * 2.0;

  return CesiumGeometry::OrientedBoundingBox(
      (childMin + childMax) / 2.0,
      glm::dmat3{xDim / 2.0, yDim / 2.0, halfAxes[2]});
}

CesiumGeometry::OrientedBoundingBox
ImplicitTilingUtilities::computeBoundingVolume(
    const CesiumGeometry::OrientedBoundingBox& rootBoundingVolume,
    const CesiumGeometry::OctreeTileID& tileID) noexcept {
  const glm::dmat3& halfAxes = rootBoundingVolume.getHalfAxes();
  const glm::dvec3& center = rootBoundingVolume.getCenter();

  double denominator = computeLevelDenominator(tileID.level);
  glm::dvec3 min = center - halfAxes[0] - halfAxes[1] - halfAxes[2];

  glm::dvec3 xDim = halfAxes[0] * 2.0 / denominator;
  glm::dvec3 yDim = halfAxes[1] * 2.0 / denominator;
  glm::dvec3 zDim = halfAxes[2] * 2.0 / denominator;
  glm::dvec3 childMin = min + xDim * double(tileID.x) +
                        yDim * double(tileID.y) + zDim * double(tileID.z);
  glm::dvec3 childMax = min + xDim * double(tileID.x + 1) +
                        yDim * double(tileID.y + 1) +
                        zDim * double(tileID.z + 1);

  return CesiumGeometry::OrientedBoundingBox(
      (childMin + childMax) / 2.0,
      glm::dmat3{xDim / 2.0, yDim / 2.0, zDim / 2.0});
}

CesiumGeospatial::S2CellBoundingVolume
ImplicitTilingUtilities::computeBoundingVolume(
    const CesiumGeospatial::S2CellBoundingVolume& rootBoundingVolume,
    const CesiumGeometry::QuadtreeTileID& tileID,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  return CesiumGeospatial::S2CellBoundingVolume(
      CesiumGeospatial::S2CellID::fromQuadtreeTileID(
          rootBoundingVolume.getCellID().getFace(),
          tileID),
      rootBoundingVolume.getMinimumHeight(),
      rootBoundingVolume.getMaximumHeight(),
      ellipsoid);
}

CesiumGeospatial::S2CellBoundingVolume
ImplicitTilingUtilities::computeBoundingVolume(
    const CesiumGeospatial::S2CellBoundingVolume& rootBoundingVolume,
    const CesiumGeometry::OctreeTileID& tileID,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  double denominator = computeLevelDenominator(tileID.level);
  double heightSize = (rootBoundingVolume.getMaximumHeight() -
                       rootBoundingVolume.getMinimumHeight()) /
                      denominator;

  double childMinHeight =
      rootBoundingVolume.getMinimumHeight() + heightSize * tileID.z;
  double childMaxHeight =
      rootBoundingVolume.getMinimumHeight() + heightSize * (tileID.z + 1);

  return CesiumGeospatial::S2CellBoundingVolume(
      CesiumGeospatial::S2CellID::fromQuadtreeTileID(
          rootBoundingVolume.getCellID().getFace(),
          QuadtreeTileID(tileID.level, tileID.x, tileID.y)),
      childMinHeight,
      childMaxHeight,
      ellipsoid);
}

CesiumGeometry::BoundingCylinderRegion
ImplicitTilingUtilities::computeBoundingVolume(
    const CesiumGeometry::BoundingCylinderRegion& rootBoundingVolume,
    const CesiumGeometry::QuadtreeTileID& tileID) noexcept {
  double denominator = computeLevelDenominator(tileID.level);

  const glm::dvec2& rootRadialBounds = rootBoundingVolume.getRadialBounds();
  const glm::dvec2& rootAngularBounds = rootBoundingVolume.getAngularBounds();

  // The angle range is "reversed" when the region sweeps over the -pi / pi
  // discontinuity line.
  bool angleReversed = rootAngularBounds.x > rootAngularBounds.y;

  double rootRadiusRange = rootRadialBounds.y - rootRadialBounds.x;
  double rootAngularRange = glm::abs(rootAngularBounds.y - rootAngularBounds.x);

  if (angleReversed) {
    // When the angle range is "reversed", the difference must be subtracted
    // from the full circle.
    rootAngularRange = CesiumUtility::Math::TwoPi - rootAngularRange;
  }

  double radiusDim = rootRadiusRange / denominator;
  double angleDim = rootAngularRange / denominator;

  double minRadius = rootRadialBounds.x + double(tileID.x) * radiusDim;
  double minAngle = rootAngularBounds.x + double(tileID.y) * angleDim;
  double maxAngle = minAngle + angleDim;

  // Ensure that the angular bounds stay within the [-pi, pi] range, but don't
  // accidentally wrap pi to -pi.
  if (minAngle >= CesiumUtility::Math::OnePi) {
    minAngle = CesiumUtility::Math::convertLongitudeRange(minAngle);
  }

  if (maxAngle > CesiumUtility::Math::OnePi) {
    maxAngle = CesiumUtility::Math::convertLongitudeRange(maxAngle);
  }

  return CesiumGeometry::BoundingCylinderRegion(
      rootBoundingVolume.getTranslation(),
      rootBoundingVolume.getRotation(),
      rootBoundingVolume.getHeight(),
      glm::dvec2(minRadius, minRadius + radiusDim),
      glm::dvec2(minAngle, maxAngle));
}

CesiumGeometry::BoundingCylinderRegion
ImplicitTilingUtilities::computeBoundingVolume(
    const CesiumGeometry::BoundingCylinderRegion& rootBoundingVolume,
    const CesiumGeometry::OctreeTileID& tileID) noexcept {
  double denominator = computeLevelDenominator(tileID.level);

  CesiumGeometry::BoundingCylinderRegion quadtreeRegion =
      ImplicitTilingUtilities::computeBoundingVolume(
          rootBoundingVolume,
          CesiumGeometry::QuadtreeTileID(tileID.level, tileID.x, tileID.y));

  double rootHeight = rootBoundingVolume.getHeight();
  double heightDim = rootHeight / denominator;

  // Due to the smaller height, the region has to be translated along its local
  // height axis.
  // Start at the center of the bottommost tiles:
  double heightOffset = -0.5 * rootHeight + 0.5 * heightDim;
  // Then offset according to tileID.z.
  heightOffset += heightDim * double(tileID.z);

  // However, this translation needs to be done before the previous translation
  // or rotation are applied.
  glm::dmat4 transform =
      CesiumGeometry::Transforms::createTranslationRotationScaleMatrix(
          rootBoundingVolume.getTranslation(),
          rootBoundingVolume.getRotation(),
          glm::dvec3(1.0)) *
      glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, 0.0, heightOffset));

  glm::dvec3 translation(0.0);
  glm::dquat rotation(1.0, 0.0, 0.0, 0.0);

  CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
      transform,
      &translation,
      &rotation,
      nullptr);

  return CesiumGeometry::BoundingCylinderRegion(
      translation,
      rotation,
      heightDim,
      quadtreeRegion.getRadialBounds(),
      quadtreeRegion.getAngularBounds());
}

double
ImplicitTilingUtilities::computeLevelDenominator(uint32_t level) noexcept {
  return static_cast<double>(1 << level);
}

QuadtreeChildren::iterator::iterator(
    const CesiumGeometry::QuadtreeTileID& parentTileID,
    bool isEnd) noexcept
    : _current(
          parentTileID.level + 1,
          parentTileID.x << 1,
          parentTileID.y << 1) {
  if (isEnd) {
    this->_current.y += 2;
  }
}

QuadtreeChildren::iterator& QuadtreeChildren::iterator::operator++() {
  // Put an indication of the child in the two low bits of `value`.
  // Bit 0 indicates left child (0) or right child (1).
  // Bit 1 indicates front child (0) or back child (1).
  uint32_t value = ((this->_current.y & 1) << 1) | (this->_current.x & 1);

  // Add one to the current value to get the value for the next child.
  // Bit cascade from addition gives us exactly what we need.
  ++value;

  // Set the tile coordinates based on the new value.
  // Bit 0 in value replaces bit 0 of the X coordinate.
  const uint32_t one = 1;
  this->_current.x = (this->_current.x & ~one) | (value & 1);

  // Value is then shifted right one bit, so it will be 0, 1, or 2.
  // 0 and 1 are the bottom or top children, while 2 indicates "end" (one past
  // the last child). So we just clear the low bit of the current Y coordinate
  // and add this shifted value to produce the new Y coordinate.
  this->_current.y = (this->_current.y & ~one) + (value >> 1);

  return *this;
}

QuadtreeChildren::iterator QuadtreeChildren::iterator::operator++(int) {
  iterator copy = *this;
  ++copy;
  return copy;
}

bool QuadtreeChildren::iterator::operator==(
    const iterator& rhs) const noexcept {
  return this->_current == rhs._current;
}

bool QuadtreeChildren::iterator::operator!=(
    const iterator& rhs) const noexcept {
  return this->_current != rhs._current;
}

OctreeChildren::iterator::iterator(
    const CesiumGeometry::OctreeTileID& parentTileID,
    bool isEnd) noexcept
    : _current(
          parentTileID.level + 1,
          parentTileID.x << 1,
          parentTileID.y << 1,
          parentTileID.z << 1) {
  if (isEnd) {
    this->_current.z += 2;
  }
}

OctreeChildren::iterator& OctreeChildren::iterator::operator++() {
  // Put an indication of the child in the three low bits of `value`.
  // Bit 0 indicates left child (0) or right child (1).
  // Bit 1 indicates front child (0) or back child (1).
  // Bit 2 indicates bottom child (0) or top child (1).
  uint32_t value = ((this->_current.z & 1) << 2) |
                   ((this->_current.y & 1) << 1) | (this->_current.x & 1);

  // Add one to the current value to get the value for the next child.
  // Bit cascade from addition gives us exactly what we need.
  ++value;

  // Set the tile coordinates based on the new value.
  // Bit 0 in value replaces bit 0 of the X coordinate.
  // Bit 1 in the value replaces bit 0 of the Y coordinate.
  const uint32_t one = 1;
  this->_current.x = (this->_current.x & ~one) | (value & 1);
  this->_current.y = (this->_current.y & ~one) | ((value >> 1) & 1);

  // Value is then shifted right one bit, so it will be 0, 1, or 2.
  // 0 and 1 are the bottom or top children, while 2 indicates "end" (one past
  // the last child). So we just clear the low bit of the current Z coordinate
  // and add this shifted value to produce the new Z coordinate.
  this->_current.z = (this->_current.z & ~one) + (value >> 2);

  return *this;
}

OctreeChildren::iterator OctreeChildren::iterator::operator++(int) {
  iterator copy = *this;
  ++copy;
  return copy;
}

bool OctreeChildren::iterator::operator==(const iterator& rhs) const noexcept {
  return this->_current == rhs._current;
}

bool OctreeChildren::iterator::operator!=(const iterator& rhs) const noexcept {
  return this->_current != rhs._current;
}

QuadtreeChildren::const_iterator
Cesium3DTilesContent::QuadtreeChildren::begin() const noexcept {
  return const_iterator(this->_tileID, false);
}

QuadtreeChildren::const_iterator QuadtreeChildren::end() const noexcept {
  return const_iterator(this->_tileID, true);
}

OctreeChildren::const_iterator
Cesium3DTilesContent::OctreeChildren::begin() const noexcept {
  return const_iterator(this->_tileID, false);
}

OctreeChildren::const_iterator OctreeChildren::end() const noexcept {
  return const_iterator(this->_tileID, true);
}

} // namespace Cesium3DTilesContent
