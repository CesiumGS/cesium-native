#include "CesiumGeometry/QuadtreeSubtreeAvailability.h"

namespace CesiumGeometry {

static const uint8_t ones_in_byte[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

static constexpr uint8_t countOnesInByte(uint8_t _byte) {
  return ones_in_byte[_byte];
}

static constexpr uint64_t countOnesInBuffer(gsl::span<const std::byte> buffer) {
  uint64_t count = 0;
  for (const std::byte& byte : buffer) {
    count += countOnesInByte((uint8_t)byte);
  }
  return count;
}

// For reference:
// https://graphics.stanford.edu/~seander/bithacks.html#Interleave64bitOps
static uint16_t getMortonIndexForBytes(uint8_t x, uint8_t y) {
  return ((x * 0x0101010101010101ULL & 0x8040201008040201ULL) *
              0x0102040810204081ULL >>
          49) &
             0x5555 |
         ((y * 0x0101010101010101ULL & 0x8040201008040201ULL) *
              0x0102040810204081ULL >>
          48) &
             0xAAAA;
}

static uint32_t getMortonIndexForShorts(uint16_t x, uint16_t y) {
  uint8_t* pFirstByteX = reinterpret_cast<uint8_t*>(&x);
  uint8_t* pFirstByteY = reinterpret_cast<uint8_t*>(&y);

  uint32_t result = 0;
  uint16_t* pFirstShortResult = reinterpret_cast<uint16_t*>(&result);

  *pFirstShortResult = getMortonIndexForBytes(*pFirstByteX, *pFirstByteY);
  *(pFirstShortResult + 1) =
      getMortonIndexForBytes(*(pFirstByteX + 1), *(pFirstByteY + 1));

  return result;
}

static uint64_t getMortonIndexForInts(uint32_t x, uint32_t y) {
  uint16_t* pFirstShortX = reinterpret_cast<uint16_t*>(&x);
  uint16_t* pFirstShortY = reinterpret_cast<uint16_t*>(&y);

  uint64_t result = 0;
  uint32_t* pFirstIntResult = reinterpret_cast<uint32_t*>(&result);

  *pFirstIntResult = getMortonIndexForShorts(*pFirstShortX, *pFirstShortY);
  *(pFirstIntResult + 1) =
      getMortonIndexForShorts(*(pFirstShortX + 1), *(pFirstShortY + 1));

  return result;
}

QuadtreeSubtreeAvailability::Subtree::Subtree(
    void* data,
    uint32_t levels_,
    uint32_t maximumLevel_)
    : levels(levels_), maximumLevel(maximumLevel_) {

  // For reference:
  // https://github.com/CesiumGS/3d-tiles/tree/3d-tiles-next/extensions/3DTILES_implicit_tiling#availability-bitstream-lengths
  // 4^levels
  size_t subtreeAvailabilitySize = 1ULL << (this->levels << 1);
  // (4^levels - 1) / 3
  size_t tileAvailabilitySize = (subtreeAvailabilitySize - 1) / 3;
  size_t tilePlusContentAvailabilitySize = tileAvailabilitySize << 1;
  size_t bufferSize = subtreeAvailabilitySize + tilePlusContentAvailabilitySize;

  this->bitstream.resize(bufferSize);
  std::memcpy(bitstream.data(), data, bufferSize);

  this->tileAvailability = gsl::span<const std::byte>(
      reinterpret_cast<const std::byte*>(bitstream.data()),
      tileAvailabilitySize);
  this->contentAvailability = gsl::span<const std::byte>(
      reinterpret_cast<const std::byte*>(
          bitstream.data() + tileAvailabilitySize),
      tileAvailabilitySize);
  this->subtreeAvailability = gsl::span<const std::byte>(
      reinterpret_cast<const std::byte*>(
          bitstream.data() + tilePlusContentAvailabilitySize),
      subtreeAvailabilitySize);
}

QuadtreeSubtreeAvailability::Node::Node(Subtree&& subtree_)
    : subtree(std::move(subtree_)) {
  size_t childNodesCount = countOnesInBuffer(this->subtree.subtreeAvailability);
  childNodes.resize(childNodesCount);
}

QuadtreeSubtreeAvailability::QuadtreeSubtreeAvailability(
    const QuadtreeTilingScheme& tilingScheme) noexcept
    : _tilingScheme(tilingScheme), _maximumLevel(0), _pRoot(nullptr) {}

uint8_t QuadtreeSubtreeAvailability::computeAvailability(
    const QuadtreeTileID& tileID) const noexcept {
  if (!this->_pRoot || tileID.level > this->_maximumLevel) {
    return 0;
  }

  uint32_t level = 0;
  Node* pNode = this->_pRoot.get();

  while (pNode && tileID.level >= level) {
    const Subtree& subtree = pNode->subtree;

    uint32_t levelDifference = std::min(tileID.level - level, subtree.levels);
    uint32_t tilesPerAxisAtLevel = 1 << levelDifference;
    uint64_t mortonIndex = getMortonIndexForInts(
        tileID.x % tilesPerAxisAtLevel,
        tileID.y % tilesPerAxisAtLevel);

    if (levelDifference < subtree.levels) {
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
      if ((uint8_t)subtree.tileAvailability[byteIndex] & bitIndex) {
        availability |= TileAvailabilityFlags::TILE_AVAILABLE;
      }

      // Check content availability.
      if ((uint8_t)subtree.tileAvailability[byteIndex] & bitIndex) {
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
    uint64_t byteIndex = mortonIndex >> 3;
    uint8_t bitIndex = static_cast<uint8_t>(mortonIndex & 7);

    gsl::span<const std::byte> clippedSubtreeAvailability =
        subtree.subtreeAvailability.subspan(0, byteIndex);
    uint8_t availabilityByte = (uint8_t)subtree.subtreeAvailability[byteIndex];

    if (availabilityByte & (1 << bitIndex)) {
      // The child subtree is available, so calculate the index it's stored in.
      uint64_t childSubtreeIndex =
          // TODO: maybe partial sums should be precomputed in the subtree
          // availability, instead of iterating through the buffer each time.
          countOnesInBuffer(clippedSubtreeAvailability) +
          countOnesInByte(availabilityByte >> (8 - bitIndex));

      pNode = pNode->childNodes[childSubtreeIndex].get();
      level += subtree.levels;
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
  // loaded. It is not reachable though, pending the load of the subtree.
  if (tileID.level == level) {
    return TileAvailabilityFlags::SUBTREE_AVAILABLE;
  }

  return 0;
}

bool QuadtreeSubtreeAvailability::addSubtree(
    const QuadtreeTileID& tileID,
    Subtree&& newSubtree) noexcept {

  if (tileID.level == 0) {
    if (this->_pRoot) {
      // The root subtree already exists.
      return false;
    } else {
      // Set the root subtree.
      this->_pRoot = std::make_unique<Node>(std::move(newSubtree));
      // TODO: assumes the maximum level of the root subtree is the overall
      // max level, is that true?
      this->_maximumLevel = this->_pRoot->subtree.maximumLevel;
      return true;
    }
  }

  if (!this->_pRoot) {
    return false;
  }

  Node* pNode = this->_pRoot.get();
  uint32_t level = 0;

  while (pNode && tileID.level > level) {
    Subtree& subtree = pNode->subtree;

    uint32_t levelDifference = tileID.level - level;

    // The given subtree to add must fall exactly at the end of an existing
    // subtree.
    if (levelDifference < subtree.levels) {
      return false;
    }

    uint32_t tilesPerAxisAtLevel = 1 << subtree.levels;
    uint64_t mortonIndex = getMortonIndexForInts(
        tileID.x % tilesPerAxisAtLevel,
        tileID.y % tilesPerAxisAtLevel);

    // TODO: consolidate duplicated code here...

    // Check if the needed child subtree exists.
    uint64_t byteIndex = mortonIndex >> 3;
    uint8_t indexWithinByte = static_cast<uint8_t>(mortonIndex & 7);

    gsl::span<const std::byte> clippedSubtreeAvailability =
        subtree.subtreeAvailability.subspan(0, byteIndex);
    uint8_t availabilityByte = (uint8_t)subtree.subtreeAvailability[byteIndex];

    if (availabilityByte & (1 << indexWithinByte)) {
      // The child subtree is available, so calculate the index it's stored in.
      uint64_t childSubtreeIndex =
          // TODO: maybe partial sums should be precomputed in the subtree
          // availability, instead of iterating through the buffer each time.
          countOnesInBuffer(clippedSubtreeAvailability) +
          countOnesInByte(availabilityByte >> (8 - indexWithinByte));

      if (levelDifference == subtree.levels) {
        if (pNode->childNodes[childSubtreeIndex]) {
          // This subtree was already added.
          // TODO: warn of error
          return false;
        }

        pNode->childNodes[childSubtreeIndex] =
            std::make_unique<Node>(std::move(newSubtree));
        return true;
      } else {
        pNode = pNode->childNodes[childSubtreeIndex].get();
        level += subtree.levels;
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