#include "CesiumGeometry/QuadtreeAvailability.h"

#include <cassert>

namespace CesiumGeometry {

// For reference:
// https://graphics.stanford.edu/~seander/bithacks.html#Interleave64bitOps
static uint16_t getMortonIndexForBytes(uint8_t a, uint8_t b) {
  return static_cast<uint16_t>(
      (((a * 0x0101010101010101ULL & 0x8040201008040201ULL) *
            0x0102040810204081ULL >>
        49) &
       0x5555) |
      (((b * 0x0101010101010101ULL & 0x8040201008040201ULL) *
            0x0102040810204081ULL >>
        48) &
       0xAAAA));
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

/**
 * @brief Get the 2D morton index for x and y. Both x and y must be no more
 * than 16 bits each.
 *
 * @param x The unsigned 16-bit integer to put in the odd bit positions.
 * @param y The unsigned 16-bit integer to put in the even bit positions.
 * @return The 32-bit unsigned morton index.
 */
static uint32_t getMortonIndex(uint32_t x, uint32_t y) {
  return getMortonIndexForShorts(
      static_cast<uint16_t>(x),
      static_cast<uint16_t>(y));
}

QuadtreeAvailability::QuadtreeAvailability(
    uint32_t subtreeLevels,
    uint32_t maximumLevel) noexcept
    : _subtreeLevels(subtreeLevels),
      _maximumLevel(maximumLevel),
      _maximumChildrenSubtrees(1U << (subtreeLevels << 1U)),
      _pRoot(nullptr) {}

bool isAvailable(
    const AvailabilityAccessor& availabilityAccessor,
    const uint32_t byteIndex,
    const uint8_t bitMask) {

  if (availabilityAccessor.isConstant()) {
    return availabilityAccessor.getConstant();
  }
  if (availabilityAccessor.isBufferView()) {
    return ((uint8_t)availabilityAccessor[byteIndex] & bitMask) != 0;
  }
  return false;
}

uint8_t computeAvailabilityImpl(
    const AvailabilitySubtree& subtree,
    const QuadtreeTileID& tileID,
    uint32_t relativeLevel) {
  AvailabilityAccessor tileAvailabilityAccessor(
      subtree.tileAvailability,
      subtree);
  AvailabilityAccessor contentAvailabilityAccessor(
      subtree.contentAvailability,
      subtree);
  AvailabilityAccessor subtreeAvailability(
      subtree.subtreeAvailability,
      subtree);

  uint32_t subtreeRelativeMask = ~(0xFFFFFFFF << relativeLevel);

  // Assume the availability info is within this subtree.
  // If this is not the case, we may return an incorrect availability.
  uint8_t availability = TileAvailabilityFlags::REACHABLE;

  uint32_t relativeMortonIndex = getMortonIndex(
      tileID.x & subtreeRelativeMask,
      tileID.y & subtreeRelativeMask);

  // For reference:
  // https://github.com/CesiumGS/3d-tiles/tree/3d-tiles-next/extensions/3DTILES_implicit_tiling#availability-bitstream-lengths
  // The below is identical to:
  // (4^levelRelativeToSubtree - 1) / 3
  uint32_t offset = ((1U << (relativeLevel << 1U)) - 1U) / 3U;

  uint32_t availabilityIndex = relativeMortonIndex + offset;
  uint32_t byteIndex = availabilityIndex >> 3;
  uint8_t bitIndex = static_cast<uint8_t>(availabilityIndex & 7);
  uint8_t bitMask = static_cast<uint8_t>(1 << bitIndex);

  // Check tile availability.
  if (isAvailable(tileAvailabilityAccessor, byteIndex, bitMask)) {
    availability |= TileAvailabilityFlags::TILE_AVAILABLE;
  }

  // Check content availability.
  if (isAvailable(contentAvailabilityAccessor, byteIndex, bitMask)) {
    availability |= TileAvailabilityFlags::CONTENT_AVAILABLE;
  }

  // If this is the 0th level within the subtree, we know this tile's
  // subtree is available and loaded.
  if (relativeLevel == 0) {
    // Setting TILE_AVAILABLE here may technically be redundant.
    availability |= TileAvailabilityFlags::TILE_AVAILABLE;
    availability |= TileAvailabilityFlags::SUBTREE_AVAILABLE;
    availability |= TileAvailabilityFlags::SUBTREE_LOADED;
  }

  return availability;
}

std::optional<uint32_t> computeChildSubtreeIndex(
    const AvailabilitySubtree& subtree,
    uint32_t childSubtreeMortonIndex) {
  AvailabilityAccessor subtreeAvailabilityAccessor(
      subtree.subtreeAvailability,
      subtree);

  // Check if the needed child subtree exists.
  bool childSubtreeAvailable = false;
  uint32_t childSubtreeIndex = 0;

  if (subtreeAvailabilityAccessor.isConstant()) {
    if (subtreeAvailabilityAccessor.getConstant()) {
      return childSubtreeMortonIndex;
    } else {
      return std::nullopt;
    }
  }

  if (subtreeAvailabilityAccessor.isBufferView()) {
    uint32_t byteIndex = childSubtreeMortonIndex >> 3;
    uint8_t bitIndex = static_cast<uint8_t>(childSubtreeMortonIndex & 7);
    uint8_t bitMask = static_cast<uint8_t>(1 << bitIndex);

    gsl::span<const std::byte> clippedSubtreeAvailability =
        subtreeAvailabilityAccessor.getBufferAccessor().subspan(0, byteIndex);
    uint8_t availabilityByte = (uint8_t)subtreeAvailabilityAccessor[byteIndex];

    childSubtreeAvailable = availabilityByte & bitMask;
    // Calculte the index the child subtree is stored in.
    if (childSubtreeAvailable) {
      childSubtreeIndex =
          // TODO: maybe partial sums should be precomputed in the subtree
          // availability, instead of iterating through the buffer each time.
          AvailabilityUtilities::countOnesInBuffer(clippedSubtreeAvailability) +
          AvailabilityUtilities::countOnesInByte(
              static_cast<uint8_t>(availabilityByte << (8 - bitIndex)));
      return childSubtreeIndex;
    } else {
      return std::nullopt;
    }
  }
  // INVALID AVAILABILITY ACCESSOR. Now cope with that...
  return std::nullopt;
}

std::optional<uint32_t> findChildNodeIndexImpl(
    const AvailabilitySubtree& subtree,
    const QuadtreeTileID& tileID,
    const uint32_t subtreeLevels) {
  uint32_t subtreeRelativeMask = ~(0xFFFFFFFF << subtreeLevels);
  uint32_t mortonIndex = getMortonIndex(
      tileID.x & subtreeRelativeMask,
      tileID.y & subtreeRelativeMask);

  AvailabilityAccessor subtreeAvailabilityAccessor(
      subtree.subtreeAvailability,
      subtree);

  std::optional<uint32_t> optionalSubtreeIndex =
      computeChildSubtreeIndex(subtree, mortonIndex);
  return optionalSubtreeIndex;
}

uint32_t computeChildSubtreeMortonIndex(
    const QuadtreeTileID& tileID,
    const uint32_t levelsLeft,
    const uint32_t subtreeLevels) {
  uint32_t subtreeRelativeMask = ~(0xFFFFFFFF << levelsLeft);
  uint32_t levelsLeftAfterNextLevel = levelsLeft - subtreeLevels;
  uint32_t childSubtreeMortonIndex = getMortonIndex(
      (tileID.x & subtreeRelativeMask) >> levelsLeftAfterNextLevel,
      (tileID.y & subtreeRelativeMask) >> levelsLeftAfterNextLevel);
  return childSubtreeMortonIndex;
}

uint8_t QuadtreeAvailability::computeAvailability(
    const QuadtreeTileID& tileID) const noexcept {

  // The root tile and root tile's subtree are implicitly available.
  if (!this->_pRoot && tileID.level == 0) {
    return TileAvailabilityFlags::TILE_AVAILABLE |
           TileAvailabilityFlags::SUBTREE_AVAILABLE;
  }

  if (!this->_pRoot || tileID.level > this->_maximumLevel) {
    return 0;
  }

  uint32_t level = 0;
  AvailabilityNode* pNode = this->_pRoot.get();

  while (pNode && pNode->subtree && tileID.level >= level) {
    const AvailabilitySubtree& subtree = *pNode->subtree;

    uint32_t levelsLeft = tileID.level - level;

    if (levelsLeft < this->_subtreeLevels) {
      uint8_t availability =
          computeAvailabilityImpl(subtree, tileID, levelsLeft);
      return availability;
    }

    uint32_t childSubtreeMortonIndex = computeChildSubtreeMortonIndex(
        tileID,
        levelsLeft,
        this->_subtreeLevels);

    std::optional<uint32_t> childSubtreeIndexOptional =
        computeChildSubtreeIndex(subtree, childSubtreeMortonIndex);

    if (childSubtreeIndexOptional.has_value()) {
      uint32_t childSubtreeIndex = childSubtreeIndexOptional.value();
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
  assert(pNode == nullptr || pNode->subtree == std::nullopt);

  // This means the tile was the root of a subtree that was available, but not
  // loaded. It is not reachable though, pending the load of the subtree. This
  // also means that the tile is available.
  if (tileID.level == level) {
    return TileAvailabilityFlags::TILE_AVAILABLE |
           TileAvailabilityFlags::SUBTREE_AVAILABLE;
  }

  return 0;
}

bool QuadtreeAvailability::addSubtree(
    const QuadtreeTileID& tileID,
    AvailabilitySubtree&& newSubtree) noexcept {

  if (tileID.level == 0) {
    if (this->_pRoot) {
      // The root subtree already exists.
      return false;
    } else {
      // Set the root subtree.
      this->_pRoot = std::make_unique<AvailabilityNode>();
      this->_pRoot->setLoadedSubtree(
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

  while (pNode && pNode->subtree && tileID.level > level) {
    AvailabilitySubtree& subtree = *pNode->subtree;

    AvailabilityAccessor subtreeAvailabilityAccessor(
        subtree.subtreeAvailability,
        subtree);

    uint32_t levelsLeft = tileID.level - level;

    // The given subtree to add must fall exactly at the end of an existing
    // subtree.
    if (levelsLeft < this->_subtreeLevels) {
      return false;
    }

    uint32_t childSubtreeMortonIndex = computeChildSubtreeMortonIndex(
        tileID,
        levelsLeft,
        this->_subtreeLevels);

    std::optional<uint32_t> childSubtreeIndexOptional =
        computeChildSubtreeIndex(subtree, childSubtreeMortonIndex);

    if (!childSubtreeIndexOptional.has_value()) {
      // TODO: warn of invalid availability
      return false;
    }
    uint32_t childSubtreeIndex = childSubtreeIndexOptional.value();

    if ((levelsLeft - this->_subtreeLevels) == 0) {
      // This is the child that the new subtree corresponds to.

      if (pNode->childNodes[childSubtreeIndex]) {
        // This subtree was already added.
        // TODO: warn of error
        return false;
      }

      pNode->childNodes[childSubtreeIndex] =
          std::make_unique<AvailabilityNode>();
      pNode->childNodes[childSubtreeIndex]->setLoadedSubtree(
          std::move(newSubtree),
          this->_maximumChildrenSubtrees);
      return true;
    } else {
      // We need to traverse this child subtree to find where to add the new
      // subtree.
      pNode = pNode->childNodes[childSubtreeIndex].get();
      level += this->_subtreeLevels;
    }
  }
  return false;
}

uint8_t QuadtreeAvailability::computeAvailability(
    const QuadtreeTileID& tileID,
    const AvailabilityNode* pNode) const noexcept {

  // If this is root of the subtree and the subtree isn't loaded yet, we can
  // atleast assume this tile and its subtree are available.
  bool subtreeLoaded = pNode && pNode->subtree;
  uint32_t relativeLevel = tileID.level % this->_subtreeLevels;
  if (!subtreeLoaded) {
    if (relativeLevel == 0) {
      return TileAvailabilityFlags::TILE_AVAILABLE |
             TileAvailabilityFlags::SUBTREE_AVAILABLE;
    }

    return 0;
  }

  const AvailabilitySubtree& subtree = *pNode->subtree;
  uint8_t availability =
      computeAvailabilityImpl(subtree, tileID, relativeLevel);
}

AvailabilityNode* QuadtreeAvailability::addNode(
    const QuadtreeTileID& tileID,
    AvailabilityNode* pParentNode) noexcept {

  if (!pParentNode || tileID.level == 0) {
    if (this->_pRoot) {
      // The root node already exists.
      return nullptr;
    } else {
      // Set the root node.
      this->_pRoot = std::make_unique<AvailabilityNode>();
      return this->_pRoot.get();
    }
  }

  // We can't insert a new child node if the parent does not have a loaded
  // subtree yet.
  if (!pParentNode->subtree) {
    return nullptr;
  }

  // The tile must fall exactly after the parent subtree.
  if ((tileID.level % this->_subtreeLevels) != 0) {
    return nullptr;
  }

  const AvailabilitySubtree& subtree = *pParentNode->subtree;
  std::optional<uint32_t> optionalSubtreeIndex =
      findChildNodeIndexImpl(subtree, tileID, this->_subtreeLevels);
  if (!optionalSubtreeIndex) {
    return nullptr;
  }
  uint32_t subtreeIndex = optionalSubtreeIndex.has_value();
  pParentNode->childNodes[subtreeIndex] = std::make_unique<AvailabilityNode>();
  return pParentNode->childNodes[subtreeIndex].get();
}

bool QuadtreeAvailability::addLoadedSubtree(
    AvailabilityNode* pNode,
    AvailabilitySubtree&& newSubtree) noexcept {
  if (!pNode || pNode->subtree) {
    return false;
  }

  pNode->setLoadedSubtree(
      std::move(newSubtree),
      this->_maximumChildrenSubtrees);

  return true;
}

std::optional<uint32_t> QuadtreeAvailability::findChildNodeIndex(
    const QuadtreeTileID& tileID,
    const AvailabilityNode* pParentNode) const {
  if (!pParentNode || !pParentNode->subtree ||
      (tileID.level % this->_subtreeLevels) != 0) {
    return std::nullopt;
  }
  const AvailabilitySubtree& subtree = *pParentNode->subtree;
  return findChildNodeIndexImpl(subtree, tileID, this->_subtreeLevels);
}

AvailabilityNode* QuadtreeAvailability::findChildNode(
    const QuadtreeTileID& tileID,
    AvailabilityNode* pParentNode) const {

  std::optional<uint32_t> childIndex =
      this->findChildNodeIndex(tileID, pParentNode);

  if (!childIndex || *childIndex >= pParentNode->childNodes.size()) {
    return nullptr;
  }

  return pParentNode->childNodes[*childIndex].get();
}

} // namespace CesiumGeometry
