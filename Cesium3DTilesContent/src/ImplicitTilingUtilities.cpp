#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/Uri.h>

#include <libmorton/morton.h>

using namespace CesiumGeometry;

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
      subtreeLevel,
      tileID.x >> levelsLeft,
      tileID.y >> levelsLeft);
}

CesiumGeometry::OctreeTileID ImplicitTilingUtilities::getSubtreeRootID(
    uint32_t subtreeLevels,
    const CesiumGeometry::OctreeTileID& tileID) noexcept {
  uint32_t subtreeLevel = tileID.level / subtreeLevels;
  uint32_t levelsLeft = tileID.level % subtreeLevels;
  return OctreeTileID(
      subtreeLevel,
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
  this->_current.x = (this->_current.x & ~1) | (value & 1);

  // Value is then shifted right one bit, so its value will be 0, 1, or 2.
  // 0 and 1 are the bottom or top children, while 2 indicates "end" (one past
  // the last child). So we just clear the low bit of the current Y coordinate
  // and add this shifted value to produce the new Y coordinate.
  this->_current.y = (this->_current.y & ~1) + (value >> 1);

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
  this->_current.x = (this->_current.x & ~1) | (value & 1);
  this->_current.y = (this->_current.y & ~1) | ((value >> 1) & 1);

  // Value is then shifted right one bit, so its value will be 0, 1, or 2.
  // 0 and 1 are the bottom or top children, while 2 indicates "end" (one past
  // the last child). So we just clear the low bit of the current Y coordinate
  // and add this shifted value to produce the new Y coordinate.
  this->_current.z = (this->_current.z & ~1) + (value >> 2);

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
