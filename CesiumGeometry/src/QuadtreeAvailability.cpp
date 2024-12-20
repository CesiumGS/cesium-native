#include <CesiumGeometry/Availability.h>
#include <CesiumGeometry/QuadtreeAvailability.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/TileAvailabilityFlags.h>
#include <CesiumUtility/Assert.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace CesiumGeometry {

namespace {

// For reference:
// https://graphics.stanford.edu/~seander/bithacks.html#Interleave64bitOps
uint16_t getMortonIndexForBytes(uint8_t a, uint8_t b) {
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

uint32_t getMortonIndexForShorts(uint16_t a, uint16_t b) {
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
uint32_t getMortonIndex(uint32_t x, uint32_t y) {
  return getMortonIndexForShorts(
      static_cast<uint16_t>(x),
      static_cast<uint16_t>(y));
}

} // namespace

QuadtreeAvailability::QuadtreeAvailability(
    uint32_t subtreeLevels,
    uint32_t maximumLevel) noexcept
    : _subtreeLevels(subtreeLevels),
      _maximumLevel(maximumLevel),
      _maximumChildrenSubtrees(1U << (subtreeLevels << 1U)),
      _pRoot(nullptr) {}

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

    AvailabilityAccessor tileAvailabilityAccessor(
        subtree.tileAvailability,
        subtree);
    AvailabilityAccessor contentAvailabilityAccessor(
        subtree.contentAvailability,
        subtree);
    AvailabilityAccessor subtreeAvailabilityAccessor(
        subtree.subtreeAvailability,
        subtree);

    uint32_t levelsLeft = tileID.level - level;
    uint32_t subtreeRelativeMask = ~(0xFFFFFFFF << levelsLeft);

    if (levelsLeft < this->_subtreeLevels) {
      // The availability info is within this subtree.
      uint8_t availability = TileAvailabilityFlags::REACHABLE;

      uint32_t relativeMortonIndex = getMortonIndex(
          tileID.x & subtreeRelativeMask,
          tileID.y & subtreeRelativeMask);

      // For reference:
      // https://github.com/CesiumGS/3d-tiles/tree/3d-tiles-next/extensions/3DTILES_implicit_tiling#availability-bitstream-lengths
      // The below is identical to:
      // (4^levelRelativeToSubtree - 1) / 3
      uint32_t offset = ((1U << (levelsLeft << 1U)) - 1U) / 3U;

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
      if (levelsLeft == 0) {
        availability |= TileAvailabilityFlags::SUBTREE_AVAILABLE;
        availability |= TileAvailabilityFlags::SUBTREE_LOADED;
      }

      return availability;
    }

    uint32_t levelsLeftAfterNextLevel = levelsLeft - this->_subtreeLevels;
    uint32_t childSubtreeMortonIndex = getMortonIndex(
        (tileID.x & subtreeRelativeMask) >> levelsLeftAfterNextLevel,
        (tileID.y & subtreeRelativeMask) >> levelsLeftAfterNextLevel);

    // Check if the needed child subtree exists.
    bool childSubtreeAvailable = false;
    uint32_t childSubtreeIndex = 0;

    if (subtreeAvailabilityAccessor.isConstant()) {
      childSubtreeAvailable = subtreeAvailabilityAccessor.getConstant();
      childSubtreeIndex = childSubtreeMortonIndex;

    } else if (subtreeAvailabilityAccessor.isBufferView()) {
      uint32_t byteIndex = childSubtreeMortonIndex >> 3;
      uint8_t bitIndex = static_cast<uint8_t>(childSubtreeMortonIndex & 7);
      uint8_t bitMask = static_cast<uint8_t>(1 << bitIndex);

      std::span<const std::byte> clippedSubtreeAvailability =
          subtreeAvailabilityAccessor.getBufferAccessor().subspan(0, byteIndex);
      uint8_t availabilityByte =
          (uint8_t)subtreeAvailabilityAccessor[byteIndex];

      childSubtreeAvailable = availabilityByte & bitMask;
      // Calculte the index the child subtree is stored in.
      if (childSubtreeAvailable) {
        childSubtreeIndex =
            // TODO: maybe partial sums should be precomputed in the subtree
            // availability, instead of iterating through the buffer each time.
            AvailabilityUtilities::countOnesInBuffer(
                clippedSubtreeAvailability) +
            AvailabilityUtilities::countOnesInByte(
                static_cast<uint8_t>(availabilityByte << (8 - bitIndex)));
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
  CESIUM_ASSERT(pNode == nullptr || pNode->subtree == std::nullopt);

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

    // TODO: consolidate duplicated code here...

    uint32_t subtreeRelativeMask = ~(0xFFFFFFFF << levelsLeft);
    uint32_t levelsLeftAfterChildren = levelsLeft - this->_subtreeLevels;
    uint32_t childSubtreeMortonIndex = getMortonIndex(
        (tileID.x & subtreeRelativeMask) >> levelsLeftAfterChildren,
        (tileID.y & subtreeRelativeMask) >> levelsLeftAfterChildren);

    // Check if the needed child subtree exists.
    bool childSubtreeAvailable = false;
    uint32_t childSubtreeIndex = 0;

    if (subtreeAvailabilityAccessor.isConstant()) {
      childSubtreeAvailable = subtreeAvailabilityAccessor.getConstant();
      childSubtreeIndex = childSubtreeMortonIndex;
    } else if (subtreeAvailabilityAccessor.isBufferView()) {
      uint32_t byteIndex = childSubtreeMortonIndex >> 3;
      uint8_t bitIndex = static_cast<uint8_t>(childSubtreeMortonIndex & 7);
      uint8_t bitMask = static_cast<uint8_t>(1 << bitIndex);

      std::span<const std::byte> clippedSubtreeAvailability =
          subtreeAvailabilityAccessor.getBufferAccessor().subspan(0, byteIndex);
      uint8_t availabilityByte =
          (uint8_t)subtreeAvailabilityAccessor[byteIndex];

      childSubtreeAvailable = availabilityByte & bitMask;
      // Calculte the index the child subtree is stored in.
      if (childSubtreeAvailable) {
        childSubtreeIndex =
            // TODO: maybe partial sums should be precomputed in the subtree
            // availability, instead of iterating through the buffer each time.
            AvailabilityUtilities::countOnesInBuffer(
                clippedSubtreeAvailability) +
            AvailabilityUtilities::countOnesInByte(
                static_cast<uint8_t>(availabilityByte << (8 - bitIndex)));
      }
    } else {
      // INVALID AVAILABILITY ACCESSOR
      return false;
    }

    if (childSubtreeAvailable) {
      if (levelsLeftAfterChildren == 0) {
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
    } else {
      // This child subtree is marked as non-available.
      // TODO: warn of invalid availability
      return false;
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

  AvailabilityAccessor tileAvailabilityAccessor(
      pNode->subtree->tileAvailability,
      *pNode->subtree);
  AvailabilityAccessor contentAvailabilityAccessor(
      pNode->subtree->contentAvailability,
      *pNode->subtree);
  AvailabilityAccessor subtreeAvailability(
      pNode->subtree->subtreeAvailability,
      *pNode->subtree);

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
  if (relativeLevel == 0) {
    // Setting TILE_AVAILABLE here may technically be redundant.
    availability |= TileAvailabilityFlags::TILE_AVAILABLE;
    availability |= TileAvailabilityFlags::SUBTREE_AVAILABLE;
    availability |= TileAvailabilityFlags::SUBTREE_LOADED;
  }

  return availability;
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

  uint32_t subtreeRelativeMask = ~(0xFFFFFFFF << this->_subtreeLevels);
  uint32_t mortonIndex = getMortonIndex(
      tileID.x & subtreeRelativeMask,
      tileID.y & subtreeRelativeMask);

  AvailabilityAccessor subtreeAvailabilityAccessor(
      pParentNode->subtree->subtreeAvailability,
      *pParentNode->subtree);

  bool subtreeAvailable = false;
  uint32_t subtreeIndex = 0;
  if (subtreeAvailabilityAccessor.isConstant()) {
    subtreeAvailable = subtreeAvailabilityAccessor.getConstant();
    subtreeIndex = mortonIndex;
  } else if (subtreeAvailabilityAccessor.isBufferView()) {
    uint32_t byteIndex = mortonIndex >> 3;
    uint8_t bitIndex = static_cast<uint8_t>(mortonIndex & 7);
    uint8_t bitMask = static_cast<uint8_t>(1 << bitIndex);

    std::span<const std::byte> clippedSubtreeAvailability =
        subtreeAvailabilityAccessor.getBufferAccessor().subspan(0, byteIndex);
    uint8_t availabilityByte = (uint8_t)subtreeAvailabilityAccessor[byteIndex];

    subtreeAvailable = availabilityByte & bitMask;

    // Calculte the index the child subtree is stored in.
    if (subtreeAvailable) {
      subtreeIndex =
          // TODO: maybe partial sums should be precomputed in the subtree
          // availability, instead of iterating through the buffer each time.
          AvailabilityUtilities::countOnesInBuffer(clippedSubtreeAvailability) +
          AvailabilityUtilities::countOnesInByte(
              static_cast<uint8_t>(availabilityByte << (8 - bitIndex)));
    } else {
      // This subtree is not supposed to be available.
      return nullptr;
    }
  }

  if (subtreeAvailable) {
    pParentNode->childNodes[subtreeIndex] =
        std::make_unique<AvailabilityNode>();
    return pParentNode->childNodes[subtreeIndex].get();
  }

  return nullptr;
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

  uint32_t subtreeRelativeMask = ~(0xFFFFFFFF << this->_subtreeLevels);
  uint32_t mortonIndex = getMortonIndex(
      tileID.x & subtreeRelativeMask,
      tileID.y & subtreeRelativeMask);

  AvailabilityAccessor subtreeAvailabilityAccessor(
      pParentNode->subtree->subtreeAvailability,
      *pParentNode->subtree);

  bool subtreeAvailable = false;
  uint32_t subtreeIndex = 0;
  if (subtreeAvailabilityAccessor.isConstant()) {
    subtreeAvailable = subtreeAvailabilityAccessor.getConstant();
    subtreeIndex = mortonIndex;
  } else if (subtreeAvailabilityAccessor.isBufferView()) {
    uint32_t byteIndex = mortonIndex >> 3;
    uint8_t bitIndex = static_cast<uint8_t>(mortonIndex & 7);
    uint8_t bitMask = static_cast<uint8_t>(1 << bitIndex);

    std::span<const std::byte> clippedSubtreeAvailability =
        subtreeAvailabilityAccessor.getBufferAccessor().subspan(0, byteIndex);
    uint8_t availabilityByte = (uint8_t)subtreeAvailabilityAccessor[byteIndex];

    subtreeAvailable = availabilityByte & bitMask;

    // Calculte the index the child subtree is stored in.
    if (subtreeAvailable) {
      subtreeIndex =
          // TODO: maybe partial sums should be precomputed in the subtree
          // availability, instead of iterating through the buffer each time.
          AvailabilityUtilities::countOnesInBuffer(clippedSubtreeAvailability) +
          AvailabilityUtilities::countOnesInByte(
              static_cast<uint8_t>(availabilityByte << (8 - bitIndex)));
    }
  }

  if (subtreeAvailable) {
    return subtreeIndex;
  }

  return std::nullopt;
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
