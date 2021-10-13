#include "CesiumGeometry/OctreeAvailability.h"

namespace CesiumGeometry {

/**
 * @brief Inserts two 0 bits of spacing between a number's bits.
 *
 * @param i A 10-bit unsigned int.
 * @return A 32-bit unsigned int.
 */
static uint32_t spread3(uint32_t i) {
  i = (i ^ (i << 16)) & 0x030000ff;
  i = (i ^ (i << 8)) & 0x0300f00f;
  i = (i ^ (i << 4)) & 0x030c30c3;
  i = (i ^ (i << 2)) & 0x09249249;

  return i;
}

/**
 * @brief Gets the morton index for x, y, and z. The x, y, and z must be no
 * more than 10 bits each.
 *
 * @param x An unsigned 10-bit number to put in places 2, 5, 8, etc.
 * @param y An unsigned 10-bit number to put in places 1, 4, 7, etc.
 * @param z An unsigned 10-bit number to put in places 0, 3, 6, etc.
 * @return The 32-bit unsigned morton index.
 */
static uint32_t getMortonIndex(uint32_t x, uint32_t y, uint32_t z) {
  return spread3(x) << 2 | spread3(y) << 1 | spread3(z);
}

OctreeAvailability::OctreeAvailability(
    const OctreeTilingScheme& tilingScheme,
    uint32_t subtreeLevels,
    uint32_t maximumLevel) noexcept
    : _tilingScheme(tilingScheme),
      _subtreeLevels(subtreeLevels),
      _maximumLevel(maximumLevel),
      _maximumChildrenSubtrees(1 << (3 * subtreeLevels)),
      _pRoot(nullptr) {}

uint8_t OctreeAvailability::computeAvailability(
    const OctreeTileID& tileID) const noexcept {
  if (!this->_pRoot || tileID.level > this->_maximumLevel) {
    return 0;
  }

  uint32_t relativeTileIdMask = 0xFFFFFFFF;

  uint32_t level = 0;
  AvailabilityNode* pNode = this->_pRoot.get();

  while (pNode && tileID.level >= level) {
    const AvailabilitySubtree& subtree = pNode->subtree;

    AvailabilityAccessor tileAvailabilityAccessor(
        subtree.tileAvailability,
        subtree);
    AvailabilityAccessor contentAvailabilityAccessor(
        subtree.contentAvailability,
        subtree);
    AvailabilityAccessor subtreeAvailabilityAccessor(
        subtree.subtreeAvailability,
        subtree);

    uint32_t levelDifference =
        std::min(tileID.level - level, this->_subtreeLevels);
    uint32_t nextLevel = level + levelDifference;
    uint32_t levelsLeftAfterNextLevel = tileID.level - nextLevel;

    if (levelDifference < this->_subtreeLevels) {
      // The availability info is within this subtree.
      uint8_t availability = TileAvailabilityFlags::REACHABLE;

      uint32_t relativeMortonIndex = getMortonIndex(
          tileID.x & relativeTileIdMask,
          tileID.y & relativeTileIdMask,
          tileID.z & relativeTileIdMask);

      // For reference:
      // https://github.com/CesiumGS/3d-tiles/tree/3d-tiles-next/extensions/3DTILES_implicit_tiling#availability-bitstream-lengths
      // The below is identical to:
      // (8^levelRelativeToSubtree - 1) / 3
      uint32_t offset = ((1 << (3 * levelDifference)) - 1) / 3;

      uint32_t availabilityIndex = relativeMortonIndex + offset;
      uint32_t byteIndex = availabilityIndex >> 3;
      uint8_t bitIndex = static_cast<uint8_t>(availabilityIndex & 7);
      uint8_t bitMask = static_cast<uint8_t>(1 << bitIndex);

      // Check tile availability.
      if ((tileAvailabilityAccessor.isConstant() &&
           tileAvailabilityAccessor.getConstant()) ||
          (tileAvailabilityAccessor.isBufferView() &&
           (uint8_t)tileAvailabilityAccessor[byteIndex] & bitMask)) {
        availability |= TileAvailabilityFlags::TILE_AVAILABLE;
      }

      // Check content availability.
      if ((contentAvailabilityAccessor.isConstant() &&
           contentAvailabilityAccessor.getConstant()) ||
          (contentAvailabilityAccessor.isBufferView() &&
           (uint8_t)contentAvailabilityAccessor[byteIndex] & bitMask)) {
        availability |= TileAvailabilityFlags::CONTENT_AVAILABLE;
      }

      // If this is the 0th level within the subtree, we know this tile's
      // subtree is available and loaded.
      if (levelDifference == 0) {
        availabilityIndex |= TileAvailabilityFlags::SUBTREE_AVAILABLE;
        availabilityIndex |= TileAvailabilityFlags::SUBTREE_LOADED;
      }

      return availability;
    }

    uint32_t childSubtreeMortonIndex = getMortonIndex(
        (tileID.x & relativeTileIdMask) >> levelsLeftAfterNextLevel,
        (tileID.y & relativeTileIdMask) >> levelsLeftAfterNextLevel,
        (tileID.z & relativeTileIdMask) >> levelsLeftAfterNextLevel);

    // Check if the needed child subtree exists.
    bool childSubtreeAvailable = false;
    uint32_t childSubtreeIndex = 0;

    if (subtreeAvailabilityAccessor.isConstant()) {
      childSubtreeAvailable = subtreeAvailabilityAccessor.getConstant();
      childSubtreeIndex = childSubtreeMortonIndex;

    } else if (subtreeAvailabilityAccessor.isBufferView()) {
      uint32_t byteIndex = childSubtreeMortonIndex >> 3;
      uint8_t bitIndex = static_cast<uint8_t>(childSubtreeMortonIndex & 7);

      gsl::span<const std::byte> clippedSubtreeAvailability =
          subtreeAvailabilityAccessor.getBufferAccessor().subspan(0, byteIndex);
      uint8_t availabilityByte =
          (uint8_t)subtreeAvailabilityAccessor[byteIndex];

      childSubtreeAvailable = availabilityByte & (1 << bitIndex);
      // Calculte the index the child subtree is stored in.
      if (childSubtreeAvailable) {
        childSubtreeIndex =
            // TODO: maybe partial sums should be precomputed in the subtree
            // availability, instead of iterating through the buffer each time.
            AvailabilityUtilities::countOnesInBuffer(
                clippedSubtreeAvailability) +
            AvailabilityUtilities::countOnesInByte(
                availabilityByte >> static_cast<uint8_t>(8 - bitIndex));
      }
    } else {
      // INVALID AVAILABILITY ACCESSOR
      return 0;
    }

    if (childSubtreeAvailable) {
      pNode = pNode->childNodes[childSubtreeIndex].get();
      level += this->_subtreeLevels;
      relativeTileIdMask >>= this->_subtreeLevels;
    } else {
      // The child subtree containing the tile id is not available.
      return TileAvailabilityFlags::REACHABLE;
    }
  }

  // This is the only case where execution should reach here. It means that a
  // subtree we need to traverse is known to be available, but it isn't yet
  // loaded.
  assert(pNode == nullptr);

  // This means the tile was the root of a subtree that was available, but not
  // loaded. It is not reachable though, pending the load of the subtree. This
  // also means that the tile is available.
  if (tileID.level == level) {
    return TileAvailabilityFlags::TILE_AVAILABLE |
           TileAvailabilityFlags::SUBTREE_AVAILABLE;
  }

  return 0;
}

bool OctreeAvailability::addSubtree(
    const OctreeTileID& tileID,
    AvailabilitySubtree&& newSubtree) noexcept {

  if (tileID.level == 0) {
    if (this->_pRoot) {
      // The root subtree already exists.
      return false;
    } else {
      // Set the root subtree.
      this->_pRoot = std::make_unique<AvailabilityNode>(
          std::move(newSubtree),
          this->_maximumChildrenSubtrees);
      return true;
    }
  }

  if (!this->_pRoot) {
    return false;
  }

  uint32_t relativeTileIdMask = 0xFFFFFFFF;

  AvailabilityNode* pNode = this->_pRoot.get();
  uint32_t level = 0;

  while (pNode && tileID.level > level) {
    AvailabilitySubtree& subtree = pNode->subtree;

    AvailabilityAccessor subtreeAvailabilityAccessor(
        subtree.subtreeAvailability,
        subtree);

    uint32_t nextLevel = level + this->_subtreeLevels;

    // The given subtree to add must fall exactly at the end of an existing
    // subtree.
    if (tileID.level < nextLevel) {
      return false;
    }

    uint32_t levelsLeftAfterChildren = tileID.level - nextLevel;
    uint32_t childSubtreeMortonIndex = getMortonIndex(
        (tileID.x & relativeTileIdMask) >> levelsLeftAfterChildren,
        (tileID.y & relativeTileIdMask) >> levelsLeftAfterChildren,
        (tileID.z & relativeTileIdMask) >> levelsLeftAfterChildren);

    // TODO: consolidate duplicated code here...

    // Check if the needed child subtree exists.
    bool childSubtreeAvailable = false;
    uint32_t childSubtreeIndex = 0;

    if (subtreeAvailabilityAccessor.isConstant()) {
      childSubtreeAvailable = subtreeAvailabilityAccessor.getConstant();
      childSubtreeIndex = childSubtreeMortonIndex;
    } else if (subtreeAvailabilityAccessor.isBufferView()) {
      uint32_t byteIndex = childSubtreeMortonIndex >> 3;
      uint8_t bitIndex = static_cast<uint8_t>(childSubtreeMortonIndex & 7);

      gsl::span<const std::byte> clippedSubtreeAvailability =
          subtreeAvailabilityAccessor.getBufferAccessor().subspan(0, byteIndex);
      uint8_t availabilityByte =
          (uint8_t)subtreeAvailabilityAccessor[byteIndex];

      childSubtreeAvailable = availabilityByte & (1 << bitIndex);
      // Calculte the index the child subtree is stored in.
      if (childSubtreeAvailable) {
        childSubtreeIndex =
            // TODO: maybe partial sums should be precomputed in the subtree
            // availability, instead of iterating through the buffer each time.
            AvailabilityUtilities::countOnesInBuffer(
                clippedSubtreeAvailability) +
            AvailabilityUtilities::countOnesInByte(
                availabilityByte >> (8 - bitIndex));
      }
    } else {
      // INVALID AVAILABILITY ACCESSOR
      return 0;
    }

    if (childSubtreeAvailable) {
      if (tileID.level == nextLevel) {
        if (pNode->childNodes[childSubtreeIndex]) {
          // This subtree was already added.
          // TODO: warn of error
          return false;
        }

        pNode->childNodes[childSubtreeIndex] =
            std::make_unique<AvailabilityNode>(
                std::move(newSubtree),
                this->_maximumChildrenSubtrees);
        return true;
      } else {
        pNode = pNode->childNodes[childSubtreeIndex].get();
        level = nextLevel;
        relativeTileIdMask >>= this->_subtreeLevels;
      }
    } else {
      // This child subtree is marked as non-available.
      // TODO: warn of invalid availability
      return false;
    }
  }

  return false;
}

} // namespace CesiumGeometry