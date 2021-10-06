#include "CesiumGeometry/QuadtreeSubtreeAvailability.h"

namespace CesiumGeometry {

// For reference:
// https://graphics.stanford.edu/~seander/bithacks.html#Interleave64bitOps
static uint16_t getMortonIndexForBytes(uint8_t a, uint8_t b) {
  return ((a * 0x0101010101010101ULL & 0x8040201008040201ULL) *
              0x0102040810204081ULL >>
          49) &
             0x5555 |
         ((b * 0x0101010101010101ULL & 0x8040201008040201ULL) *
              0x0102040810204081ULL >>
          48) &
             0xAAAA;
}

static uint32_t getMortonIndexForShorts(uint16_t a, uint16_t b) {
  uint8_t* pFirstByteA = reinterpret_cast<uint8_t*>(&a);
  uint8_t* pFirstByteB = reinterpret_cast<uint8_t*>(&b);

  uint32_t result = 0;
  uint16_t* pFirstShortResult = reinterpret_cast<uint16_t*>(&result);

  *pFirstShortResult = getMortonIndexForBytes(*pFirstByteA, *pFirstByteB);
  *(pFirstShortResult + 1) =
      getMortonIndexForBytes(*(pFirstByteA + 1), *(pFirstByteB + 1));

  return result;
}

static uint64_t getMortonIndexForInts(uint32_t a, uint32_t b) {
  uint16_t* pFirstShortA = reinterpret_cast<uint16_t*>(&a);
  uint16_t* pFirstShortB = reinterpret_cast<uint16_t*>(&b);

  uint64_t result = 0;
  uint32_t* pFirstIntResult = reinterpret_cast<uint32_t*>(&result);

  *pFirstIntResult = getMortonIndexForShorts(*pFirstShortA, *pFirstShortB);
  *(pFirstIntResult + 1) =
      getMortonIndexForShorts(*(pFirstShortA + 1), *(pFirstShortB + 1));

  return result;
}

QuadtreeSubtreeAvailability::QuadtreeSubtreeAvailability(
    const QuadtreeTilingScheme& tilingScheme,
    uint32_t subtreeLevels,
    uint32_t maximumLevel) noexcept
    : _tilingScheme(tilingScheme),
      _subtreeLevels(subtreeLevels),
      _maximumLevel(maximumLevel),
      _maximumChildrenSubtrees(1ULL << (subtreeLevels << 1)),
      _pRoot(nullptr) {}

uint8_t QuadtreeSubtreeAvailability::computeAvailability(
    const QuadtreeTileID& tileID) const noexcept {
  if (!this->_pRoot || tileID.level > this->_maximumLevel) {
    return 0;
  }

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

    uint32_t relativeTileIdMask = ~(0xFFFFFFFF << level);
    uint32_t levelDifference =
        std::min(tileID.level - level, this->_subtreeLevels);
    uint32_t nextLevel = level + levelDifference;
    uint32_t levelsLeftAfterNextLevel = tileID.level - nextLevel;

    uint64_t mortonIndex = getMortonIndexForInts(
        (relativeTileIdMask & tileID.y) >> levelsLeftAfterNextLevel,
        (relativeTileIdMask & tileID.x) >> levelsLeftAfterNextLevel);

    if (levelDifference < this->_subtreeLevels) {
      // The availability info is within this subtree.
      uint8_t availability = TileAvailabilityFlags::REACHABLE;

      // For reference:
      // https://github.com/CesiumGS/3d-tiles/tree/3d-tiles-next/extensions/3DTILES_implicit_tiling#availability-bitstream-lengths
      // The below is identical to:
      // (4^levelRelativeToSubtree - 1) / 3
      uint64_t offset = ((1 << (levelDifference << 1)) - 1) / 3;

      uint64_t availabilityIndex = mortonIndex + offset;
      uint64_t byteIndex = availabilityIndex >> 3;
      uint8_t bitIndex = static_cast<uint8_t>(availabilityIndex & 7);
      uint8_t bitMask = 1 << bitIndex;

      // Check tile availability.
      if (tileAvailabilityAccessor.isConstant() &&
              tileAvailabilityAccessor.getConstant() ||
          tileAvailabilityAccessor.isBufferView() &&
              (uint8_t)tileAvailabilityAccessor[byteIndex] & bitMask) {
        availability |= TileAvailabilityFlags::TILE_AVAILABLE;
      }

      // Check content availability.
      if (contentAvailabilityAccessor.isConstant() &&
              contentAvailabilityAccessor.getConstant() ||
          contentAvailabilityAccessor.isBufferView() &&
              (uint8_t)contentAvailabilityAccessor[byteIndex] & bitMask) {
        availability |= TileAvailabilityFlags::CONTENT_AVAILABLE;
      }

      // If this is the 0th level within the subtree, we know this tile's
      // subtree is available and loaded.
      if (levelDifference == 0) {
        availabilityIndex |= TileAvailabilityFlags::SUBTREE_LOADED;
      }

      return availability;
    }

    // Check if the needed child subtree exists.
    bool childSubtreeAvailable = false;
    uint64_t childSubtreeIndex = 0;

    if (subtreeAvailabilityAccessor.isConstant()) {
      childSubtreeAvailable = subtreeAvailabilityAccessor.getConstant();
      childSubtreeIndex = mortonIndex;

    } else if (subtreeAvailabilityAccessor.isBufferView()) {
      uint64_t byteIndex = mortonIndex >> 3;
      uint8_t bitIndex = static_cast<uint8_t>(mortonIndex & 7);

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
      pNode = pNode->childNodes[childSubtreeIndex].get();
      level += this->_subtreeLevels;
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

bool QuadtreeSubtreeAvailability::addSubtree(
    const QuadtreeTileID& tileID,
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

  AvailabilityNode* pNode = this->_pRoot.get();
  uint32_t level = 0;

  while (pNode && tileID.level > level) {
    AvailabilitySubtree& subtree = pNode->subtree;

    AvailabilityAccessor subtreeAvailabilityAccessor(
        subtree.subtreeAvailability,
        subtree);

    uint32_t relativeTileIdMask = ~(0xFFFFFFFF << level);
    uint32_t nextLevel = level + this->_subtreeLevels;

    // The given subtree to add must fall exactly at the end of an existing
    // subtree.
    if (tileID.level < nextLevel) {
      return false;
    }

    uint32_t levelsLeftAfterChildren = tileID.level - nextLevel;
    uint64_t mortonIndex = getMortonIndexForInts(
        (relativeTileIdMask & tileID.y) >> levelsLeftAfterChildren,
        (relativeTileIdMask & tileID.x) >> levelsLeftAfterChildren);

    // TODO: consolidate duplicated code here...

    // Check if the needed child subtree exists.
    bool childSubtreeAvailable = false;
    uint64_t childSubtreeIndex = 0;

    if (subtreeAvailabilityAccessor.isConstant()) {
      childSubtreeAvailable = subtreeAvailabilityAccessor.getConstant();
      childSubtreeIndex = mortonIndex;
    } else if (subtreeAvailabilityAccessor.isBufferView()) {
      uint64_t byteIndex = mortonIndex >> 3;
      uint8_t bitIndex = static_cast<uint8_t>(mortonIndex & 7);

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