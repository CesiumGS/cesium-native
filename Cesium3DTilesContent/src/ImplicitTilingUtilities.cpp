#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/Uri.h>

#include <libmorton/morton.h>

using namespace CesiumGeometry;

namespace Cesium3DTilesContent {

/*static*/ std::string ImplicitTilingUtilities::resolveUrl(
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

/*static*/ std::string ImplicitTilingUtilities::resolveUrl(
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

/*static*/ uint64_t ImplicitTilingUtilities::computeRelativeMortonIndex(
    const QuadtreeTileID& subtreeID,
    const QuadtreeTileID& tileID) {
  uint32_t relativeTileLevel = tileID.level - subtreeID.level;
  return libmorton::morton2D_64_encode(
      tileID.x - (subtreeID.x << relativeTileLevel),
      tileID.y - (subtreeID.y << relativeTileLevel));
}

/*static*/ uint64_t ImplicitTilingUtilities::computeRelativeMortonIndex(
    const OctreeTileID& subtreeID,
    const OctreeTileID& tileID) {
  uint32_t relativeTileLevel = tileID.level - subtreeID.level;
  return libmorton::morton3D_64_encode(
      tileID.x - (subtreeID.x << relativeTileLevel),
      tileID.y - (subtreeID.y << relativeTileLevel),
      tileID.z - (subtreeID.z << relativeTileLevel));
}

QuadtreeChildIterator::QuadtreeChildIterator(
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

QuadtreeChildIterator& QuadtreeChildIterator::operator++() {
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

QuadtreeChildIterator QuadtreeChildIterator::operator++(int) {
  QuadtreeChildIterator copy = *this;
  ++copy;
  return copy;
}

bool QuadtreeChildIterator::operator==(
    const QuadtreeChildIterator& rhs) const noexcept {
  return this->_current == rhs._current;
}

bool QuadtreeChildIterator::operator!=(
    const QuadtreeChildIterator& rhs) const noexcept {
  return this->_current != rhs._current;
}

OctreeChildIterator::OctreeChildIterator(
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

OctreeChildIterator& OctreeChildIterator::operator++() {
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

OctreeChildIterator OctreeChildIterator::operator++(int) {
  OctreeChildIterator copy = *this;
  ++copy;
  return copy;
}

bool OctreeChildIterator::operator==(
    const OctreeChildIterator& rhs) const noexcept {
  return this->_current == rhs._current;
}

bool OctreeChildIterator::operator!=(
    const OctreeChildIterator& rhs) const noexcept {
  return this->_current != rhs._current;
}

QuadtreeChildIterator ImplicitTilingUtilities::childrenBegin(
    const CesiumGeometry::QuadtreeTileID& tileID) noexcept {
  return QuadtreeChildIterator(tileID, false);
}

QuadtreeChildIterator ImplicitTilingUtilities::childrenEnd(
    const CesiumGeometry::QuadtreeTileID& tileID) noexcept {
  return QuadtreeChildIterator(tileID, true);
}

OctreeChildIterator ImplicitTilingUtilities::childrenBegin(
    const CesiumGeometry::OctreeTileID& tileID) noexcept {
  return OctreeChildIterator(tileID, false);
}

OctreeChildIterator ImplicitTilingUtilities::childrenEnd(
    const CesiumGeometry::OctreeTileID& tileID) noexcept {
  return OctreeChildIterator(tileID, true);
}

std::array<QuadtreeTileID, 4>
ImplicitTilingUtilities::getChildTileIDs(const QuadtreeTileID& parentTileID) {
  uint32_t level = parentTileID.level + 1;
  uint32_t x = parentTileID.x << 1;
  uint32_t y = parentTileID.y << 1;
  return std::array<QuadtreeTileID, 4>{
      QuadtreeTileID(level, x, y),
      QuadtreeTileID(level, x + 1, y),
      QuadtreeTileID(level, x, y + 1),
      QuadtreeTileID(level, x + 1, y + 1)};
}

std::array<OctreeTileID, 8>
ImplicitTilingUtilities::getChildTileIDs(const OctreeTileID& parentTileID) {
  uint32_t level = parentTileID.level + 1;
  uint32_t x = parentTileID.x << 1;
  uint32_t y = parentTileID.y << 1;
  uint32_t z = parentTileID.z << 1;
  return std::array<OctreeTileID, 8>{
      OctreeTileID(level, x, y, z),
      OctreeTileID(level, x + 1, y, z),
      OctreeTileID(level, x, y + 1, z),
      OctreeTileID(level, x + 1, y + 1, z),
      OctreeTileID(level, x, y, z + 1),
      OctreeTileID(level, x + 1, y, z + 1),
      OctreeTileID(level, x, y + 1, z + 1),
      OctreeTileID(level, x + 1, y + 1, z + 1)};
}

} // namespace Cesium3DTilesContent
