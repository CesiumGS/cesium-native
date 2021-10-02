#include "CesiumGeometry/QuadtreeSubtreeAvailability.h"

namespace CesiumGeometry {

static const uint8_t ones_in_byte[] = {
    0,
    1,
    1,
    2,
    1,
    2,
    2,
    3,
    1,
    2,
    2,
    3,
    2,
    3,
    3,
    4,
    1,
    2,
    2,
    3,
    2,
    3,
    3,
    4,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    1,
    2,
    2,
    3,
    2,
    3,
    3,
    4,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    1,
    2,
    2,
    3,
    2,
    3,
    3,
    4,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    4,
    5,
    5,
    6,
    5,
    6,
    6,
    7,
    1,
    2,
    2,
    3,
    2,
    3,
    3,
    4,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    4,
    5,
    5,
    6,
    5,
    6,
    6,
    7,
    2,
    3,
    3,
    4,
    3,
    4,
    4,
    5,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    4,
    5,
    5,
    6,
    5,
    6,
    6,
    7,
    3,
    4,
    4,
    5,
    4,
    5,
    5,
    6,
    4,
    5,
    5,
    6,
    5,
    6,
    6,
    7,
    4,
    5,
    5,
    6,
    5,
    6,
    6,
    7,
    5,
    6,
    6,
    7,
    6,
    7,
    7,
    8};

static constexpr uint8_t countOnesInByte(uint8_t _byte) {
  return ones_in_byte[_byte];
}

static constexpr uint8_t countOnesInShort(uint16_t _short) {
  uint8_t* pFirstByte = reinterpret_cast<uint8_t*>(&_short);
  return 
      countOnesInByte(*pFirstByte) + 
      countOnesInByte(*(pFirstByte + 1));
}

static constexpr uint8_t countOnesInInt(uint32_t _int) {
  uint16_t* pFirtShort = reinterpret_cast<uint16_t*>(&_int);
  return  
      countOnesInShort(*pFirtShort) +
      countOnesInShort(*(pFirtShort + 1));
}

static constexpr uint8_t countOnesInLong(uint64_t _long) {
  uint32_t* pFirstInt = reinterpret_cast<uint32_t*>(&_long);
  return
      countOnesInInt(*pFirstInt) +
      countOnesInInt(*(pFirstInt + 1));
}

// For reference: https://graphics.stanford.edu/~seander/bithacks.html#Interleave64bitOps
static constexpr uint16_t getMortonIndex(uint8_t x, uint8_t y) {
  return 
      ((x * 0x0101010101010101ULL & 0x8040201008040201ULL) * 
      0x0102040810204081ULL >> 49) & 0x5555 |
      ((y * 0x0101010101010101ULL & 0x8040201008040201ULL) * 
      0x0102040810204081ULL >> 48) & 0xAAAA;
}

QuadtreeSubtreeAvailability::QuadtreeSubtreeAvailability(
    const QuadtreeTilingScheme& tilingScheme,
    uint32_t maximumLevel) noexcept
  : _tilingScheme(tilingScheme),
    _maximumLevel(maximumLevel),
    _pRoot(nullptr) {}

bool 
QuadtreeSubtreeAvailability::isTileAvailable(
    const QuadtreeTileID& tileID) const noexcept {
  uint32_t level = 0;
  Node* pNode = this->_pRoot.get();

  while (pNode && tileID.level != level) {
    const Subtree& subtree = pNode->subtree;

    if (tileID.level < level + 3) {
      // The availability info is within this subtree.
    
      uint32_t tilesPerAxisAtLevel = 1 << tileID.level;

      // If the tileID's level is correct, its x and y are in [0, 3].
      // The morton index will be in [0], [0, 3], or [0, 15].
      uint8_t mortonIndex = 
          static_cast<uint8_t>(
            getMortonIndex(
              static_cast<uint8_t>(tileID.x % tilesPerAxisAtLevel), 
              static_cast<uint8_t>(tileID.y % tilesPerAxisAtLevel)));

      const static uint8_t offsetBySubtreeLevel[] = { 0, 1, 4 };

      // This index will be in [0, 20]
      uint8_t availabilityIndex = 
          mortonIndex + offsetBySubtreeLevel[tileID.level - level];

      return subtree.tileAvailability & (1 << (20 - availabilityIndex));
    }

    level += 3;

    uint32_t levelDifference = tileID.level - level;

    // Check if the needed child subtree exists.
    // Morton index here will be in [0, 63].
    uint16_t mortonIndex = 
        getMortonIndex(
          static_cast<uint8_t>(tileID.x >> levelDifference),
          static_cast<uint8_t>(tileID.y >> levelDifference));

    uint64_t clippedSubtreeAvailability = 
        subtree.subtreeAvailability >> (63 - mortonIndex);
    
    if (clippedSubtreeAvailability & 1) {
      // The child subtree is available, so calculate the index it's stored in.
      pNode = 
          pNode->
            childNodes[countOnesInLong(clippedSubtreeAvailability) - 1].get();
    } else {
      // The child subtree containing the tile id is not available.
      return false;
    }
  }

  assert(false);
  // TODO: should execution ever reach here?
  return false;
}

void QuadtreeSubtreeAvailability::addSubtree(
    const QuadtreeTileID& tileID, const Subtree& newSubtree) noexcept {
  // Only levels that are a multiple of 3 can hold subtrees.
  if (tileID.level % 3) {
    return;
  }

  if (tileID.level == 0) {
    this->_pRoot = std::make_unique<Node>();
    this->_pRoot->subtree = newSubtree;
    return;
  }

  uint32_t level = 3;
  Node* pNode = this->_pRoot.get();

  while (pNode && tileID.level != level) {
    if (tileID.level == level) {

    }

    level += 3;

    uint32_t levelDifference = tileID.level - level;

    // Check if the needed child subtree exists.
    // Morton index here will be in [0, 63].
    uint16_t mortonIndex = 
        getMortonIndex(
          static_cast<uint8_t>(tileID.x >> levelDifference),
          static_cast<uint8_t>(tileID.y >> levelDifference));

    uint64_t clippedSubtreeAvailability = 
        subtree.subtreeAvailability >> (63 - mortonIndex);
    
    if (clippedSubtreeAvailability & 1) {
      // The child subtree is available, so calculate the index it's stored in.
      pNode = 
          pNode->
            childNodes[countOnesInLong(clippedSubtreeAvailability) - 1].get();
    } else {
      // The child subtree containing the tile id is not available.
      return false;
    }
  }
}

} // namespace CesiumGeometry