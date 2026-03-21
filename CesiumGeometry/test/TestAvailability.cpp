
#include <CesiumGeometry/Availability.h>
#include <CesiumGeometry/OctreeAvailability.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeAvailability.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/TileAvailabilityFlags.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

using namespace CesiumGeometry;

TEST_CASE("Test AvailabilityUtilities") {
  SUBCASE("Test countOnesInByte") {
    uint8_t byte = static_cast<uint8_t>(0xFF);
    for (uint8_t i = 0; i <= 8; ++i) {
      REQUIRE(
          AvailabilityUtilities::countOnesInByte(
              static_cast<uint8_t>(byte >> i)) == (8 - i));
    }
  }

  SUBCASE("Test countOnesInBuffer") {
    std::vector<std::byte> buffer(64);
    for (size_t i = 0; i < 64U; ++i) {
      buffer[i] = static_cast<std::byte>(0xFC);
    }

    // Each byte is 0xFC which has 6 ones.
    // This means there are 6 x 64 = 384 ones total in the buffer.
    uint32_t onesInBuffer = AvailabilityUtilities::countOnesInBuffer(
        std::span<std::byte>(&buffer[0], 64));
    REQUIRE(onesInBuffer == 384U);
  }
}

TEST_CASE("Test AvailabilityAccessor") {
  std::vector<std::byte> availabilityBuffer(64);
  for (size_t i = 0; i < 64U; ++i) {
    availabilityBuffer[i] = static_cast<std::byte>(0xFC);
  }

  AvailabilitySubtree subtree{
      ConstantAvailability{true},
      SubtreeBufferView{0, 64, 0},
      ConstantAvailability{false},
      {std::move(availabilityBuffer)}};

  AvailabilityAccessor tileAvailabilityAccessor(
      subtree.tileAvailability,
      subtree);
  AvailabilityAccessor contentAvailabilityAccessor(
      subtree.contentAvailability,
      subtree);
  AvailabilityAccessor subtreeAvailabilityAccessor(
      subtree.subtreeAvailability,
      subtree);

  SUBCASE("Test constant availability") {
    REQUIRE(tileAvailabilityAccessor.isConstant());
    REQUIRE(tileAvailabilityAccessor.getConstant());
    REQUIRE(!tileAvailabilityAccessor.isBufferView());
    REQUIRE(subtreeAvailabilityAccessor.isConstant());
    REQUIRE(!subtreeAvailabilityAccessor.getConstant());
    REQUIRE(!subtreeAvailabilityAccessor.isBufferView());
  }

  SUBCASE("Test buffer availability") {
    REQUIRE(!contentAvailabilityAccessor.isConstant());
    REQUIRE(contentAvailabilityAccessor.isBufferView());
    REQUIRE(contentAvailabilityAccessor.size() == 64);
    for (size_t i = 0; i < 64U; ++i) {
      REQUIRE(contentAvailabilityAccessor[i] == static_cast<std::byte>(0xFC));
    }
  }

  SUBCASE("Test combined buffer availability") {
    // Now try sharing a single buffer between multiple views.
    subtree.tileAvailability = SubtreeBufferView{0, 32, 0};
    subtree.contentAvailability = SubtreeBufferView{32, 32, 0};

    // Recreate accessors.
    tileAvailabilityAccessor =
        AvailabilityAccessor(subtree.tileAvailability, subtree);
    contentAvailabilityAccessor =
        AvailabilityAccessor(subtree.contentAvailability, subtree);

    REQUIRE(!tileAvailabilityAccessor.isConstant());
    REQUIRE(tileAvailabilityAccessor.isBufferView());
    REQUIRE(tileAvailabilityAccessor.size() == 32U);

    REQUIRE(!contentAvailabilityAccessor.isConstant());
    REQUIRE(contentAvailabilityAccessor.isBufferView());
    REQUIRE(contentAvailabilityAccessor.size() == 32U);

    for (size_t i = 0; i < 32U; ++i) {
      REQUIRE(tileAvailabilityAccessor[i] == static_cast<std::byte>(0xFC));
      REQUIRE(contentAvailabilityAccessor[i] == static_cast<std::byte>(0xFC));
    }
  }
}

TEST_CASE("Test OctreeAvailability") {
  // We will test with an octree availability subtree with 3 levels.

  // All tiles in the root subtree will have tile availability.

  // The content availability will require a bitstream with 73 bits, but we
  // will need to align to an 8-byte boundary. So it will take 16 bytes.

  // The subtree availability bitstream will require 512 bits (64 bytes).

  // Tiles with morton index 12, 13, 14, and 15 will not have content.
  // These are tiles 3, 4, 5, 6 in level 2.
  // These are tile IDs (2, 1, 1, 0), (2, 0, 0, 1), (2, 1, 0, 1), and
  // (2, 0, 1, 1).
  std::vector<std::byte> contentAvailabilityBuffer(16);

  // Fill the first 72 bits with ones.
  for (size_t i = 0; i < 9U; ++i) {
    contentAvailabilityBuffer[i] = static_cast<std::byte>(0xFF);
  }

  // Fill just the 72nd bit with one.
  contentAvailabilityBuffer[9] = static_cast<std::byte>(0x01);

  // Set zeroes for bits 12, 13, 14, and 15.
  contentAvailabilityBuffer[1] = static_cast<std::byte>(0x0F);

  OctreeTileID unavailableContentIds[] = {
      OctreeTileID(2, 1, 1, 0),
      OctreeTileID(2, 0, 0, 1),
      OctreeTileID(2, 1, 0, 1),
      OctreeTileID(2, 0, 1, 1)};

  // Child subtrees 44, 45, 46, and 47 will be unavailable.
  // These correspond to tile IDs (3, 2, 0, 3), (3, 3, 0, 3), (3, 2, 1, 3), and
  // (3, 3, 1, 3).
  std::vector<std::byte> subtreeAvailabilityBuffer(64);

  // Fill all bits with ones.
  for (size_t i = 0; i < 64U; ++i) {
    subtreeAvailabilityBuffer[i] = static_cast<std::byte>(0xFF);
  }

  // Fill bits 44, 45, 46, and 47 with zeroes.
  subtreeAvailabilityBuffer[5] = static_cast<std::byte>(0x0F);

  OctreeTileID unavailableSubtreeIds[]{
      OctreeTileID(3, 2, 0, 3),
      OctreeTileID(3, 3, 0, 3),
      OctreeTileID(3, 2, 1, 3),
      OctreeTileID(3, 3, 1, 3)};

  AvailabilitySubtree subtree{
      ConstantAvailability{true},
      SubtreeBufferView{0, 16, 0},
      SubtreeBufferView{0, 64, 1},
      {contentAvailabilityBuffer, subtreeAvailabilityBuffer}};

  OctreeAvailability octreeAvailability(3, 5);
  octreeAvailability.addSubtree(OctreeTileID(0, 0, 0, 0), std::move(subtree));

  AvailabilityNode* pParentNode = octreeAvailability.getRootNode();

  SUBCASE("Test tile and content availability") {
    for (uint32_t level = 0; level < 3U; ++level) {
      for (uint32_t z = 0; z < (1U << level); ++z) {
        for (uint32_t y = 0; y < (1U << level); ++y) {
          for (uint32_t x = 0; x < (1U << level); ++x) {
            OctreeTileID id(level, x, y, z);
            uint8_t availability = octreeAvailability.computeAvailability(id);
            uint8_t availability2 =
                octreeAvailability.computeAvailability(id, pParentNode);

            REQUIRE(availability == availability2);

            // All tiles should be available.
            REQUIRE_UNARY(availability & TileAvailabilityFlags::TILE_AVAILABLE);

            // Whether the content should be available
            bool contentShouldBeAvailable = true;
            for (const OctreeTileID& tileID : unavailableContentIds) {
              if (tileID == id) {
                contentShouldBeAvailable = false;
                break;
              }
            }

            REQUIRE(
                (bool)(availability & TileAvailabilityFlags::CONTENT_AVAILABLE) ==
                contentShouldBeAvailable);
          }
        }
      }
    }
  }

  SUBCASE("Test children subtree availability") {
    // Check child subtree availability, none are loaded yet.
    uint32_t componentLengthAtLevel = 1U << 3;
    for (uint32_t z = 0; z < componentLengthAtLevel; ++z) {
      for (uint32_t y = 0; y < componentLengthAtLevel; ++y) {
        for (uint32_t x = 0; x < componentLengthAtLevel; ++x) {
          OctreeTileID id(3, x, y, z);
          uint8_t availability = octreeAvailability.computeAvailability(id);
          std::optional<uint32_t> childIndex =
              octreeAvailability.findChildNodeIndex(id, pParentNode);

          bool subtreeShouldBeAvailable = true;
          for (const OctreeTileID& tileID : unavailableSubtreeIds) {
            if (tileID == id) {
              subtreeShouldBeAvailable = false;
              break;
            }
          }

          REQUIRE(
              (bool)(availability & TileAvailabilityFlags::SUBTREE_AVAILABLE) ==
              subtreeShouldBeAvailable);

          REQUIRE((childIndex != std::nullopt) == subtreeShouldBeAvailable);
        }
      }
    }
  }

  SUBCASE("Test children subtree loaded flag") {
    // Mock loaded child subtrees for tile IDs (3, 0, 0, 0), (3, 0, 1, 0), (3,
    // 0, 2, 0), and (3, 1, 2, 1).
    OctreeTileID mockChildrenSubtreeIds[]{
        OctreeTileID(3, 0, 0, 0),
        OctreeTileID(3, 0, 1, 0),
        OctreeTileID(3, 0, 2, 0),
        OctreeTileID(3, 1, 2, 1)};

    SUBCASE("Use addSubtree(id, newSubtree)") {
      for (const OctreeTileID& mockChildrenSubtreeId : mockChildrenSubtreeIds) {
        AvailabilitySubtree childSubtree{
            ConstantAvailability{true},
            ConstantAvailability{true},
            ConstantAvailability{false},
            {}};

        octreeAvailability.addSubtree(
            mockChildrenSubtreeId,
            std::move(childSubtree));
      }
    }

    SUBCASE("Use addNode and addLoadedSubtree") {
      for (const OctreeTileID& mockChildrenSubtreeId : mockChildrenSubtreeIds) {
        AvailabilitySubtree childSubtree{
            ConstantAvailability{true},
            ConstantAvailability{true},
            ConstantAvailability{false},
            {}};

        AvailabilityNode* pNode =
            octreeAvailability.addNode(mockChildrenSubtreeId, pParentNode);

        REQUIRE(pNode != nullptr);
        octreeAvailability.addLoadedSubtree(pNode, std::move(childSubtree));
      }
    }

    // Check that the correct child subtrees are noted to be loaded.
    uint32_t componentLengthAtLevel = 1U << 3;
    for (uint32_t z = 0; z < componentLengthAtLevel; ++z) {
      for (uint32_t y = 0; y < componentLengthAtLevel; ++y) {
        for (uint32_t x = 0; x < componentLengthAtLevel; ++x) {
          OctreeTileID id(3, x, y, z);

          // Test computeAvailability
          uint8_t availability = octreeAvailability.computeAvailability(id);

          // Test findChildNode
          AvailabilityNode* pChildNode =
              octreeAvailability.findChildNode(id, pParentNode);

          bool subtreeShouldBeLoaded = false;
          for (const OctreeTileID& tileID : mockChildrenSubtreeIds) {
            if (tileID == id) {
              subtreeShouldBeLoaded = true;
              break;
            }
          }

          REQUIRE(
              (bool)(availability & TileAvailabilityFlags::SUBTREE_LOADED) ==
              subtreeShouldBeLoaded);

          REQUIRE((pChildNode != nullptr) == subtreeShouldBeLoaded);
        }
      }
    }
  }
}

TEST_CASE("Test QuadtreeAvailability") {
  // We will test with a quadtree availability subtree with 3 levels.

  // All tiles in the root subtree will be available.

  // The content availability will require a bitstream with 21 bits, but we
  // will need to byte-align to 8 bytes.

  // The subtree availability bitstream will require 64 bits (exactly 8 bytes).

  // Tiles with morton index 12, 13, 14, and 15 will not have content.
  // These are tiles 7, 8, 9, 10 in level 2.
  // These are tile IDs (2, 3, 1), (2, 0, 2), (2, 1, 2), and (2, 0, 3).
  std::vector<std::byte> contentAvailabilityBuffer = {
      static_cast<std::byte>(0xFF),
      static_cast<std::byte>(0x0F),
      static_cast<std::byte>(0x3F),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00)};

  QuadtreeTileID unavailableContentIds[] = {
      QuadtreeTileID(2, 3, 1),
      QuadtreeTileID(2, 0, 2),
      QuadtreeTileID(2, 1, 2),
      QuadtreeTileID(2, 0, 3)};

  // Child subtrees 44, 45, 46, and 47 will be unavailable.
  // These correspond to tile IDs (3, 2, 6), (3, 3, 6), (3, 2, 7), and
  // (3, 3, 7).
  std::vector<std::byte> subtreeAvailabilityBuffer = {
      static_cast<std::byte>(0xFF),
      static_cast<std::byte>(0xFF),
      static_cast<std::byte>(0xFF),
      static_cast<std::byte>(0xFF),
      static_cast<std::byte>(0xFF),
      static_cast<std::byte>(0x0F),
      static_cast<std::byte>(0xFF),
      static_cast<std::byte>(0xFF)};

  QuadtreeTileID unavailableSubtreeIds[]{
      QuadtreeTileID(3, 2, 6),
      QuadtreeTileID(3, 3, 6),
      QuadtreeTileID(3, 2, 7),
      QuadtreeTileID(3, 3, 7)};

  AvailabilitySubtree subtree{
      ConstantAvailability{true},
      SubtreeBufferView{0, 8, 0},
      SubtreeBufferView{0, 8, 1},
      {contentAvailabilityBuffer, subtreeAvailabilityBuffer}};

  QuadtreeAvailability quadtreeAvailability(3, 5);
  quadtreeAvailability.addSubtree(QuadtreeTileID(0, 0, 0), std::move(subtree));

  AvailabilityNode* pParentNode = quadtreeAvailability.getRootNode();

  SUBCASE("Test tile and content availability") {
    for (uint32_t level = 0; level < 3U; ++level) {
      for (uint32_t y = 0; y < (1U << level); ++y) {
        for (uint32_t x = 0; x < (1U << level); ++x) {
          QuadtreeTileID id(level, x, y);
          uint8_t availability = quadtreeAvailability.computeAvailability(id);
          uint8_t availability2 =
              quadtreeAvailability.computeAvailability(id, pParentNode);

          REQUIRE(availability == availability2);

          // All tiles should be available.
          REQUIRE_UNARY(availability & TileAvailabilityFlags::TILE_AVAILABLE);

          // Whether the content should be available
          bool contentShouldBeAvailable = true;
          for (const QuadtreeTileID& tileID : unavailableContentIds) {
            if (tileID == id) {
              contentShouldBeAvailable = false;
              break;
            }
          }

          REQUIRE(
              (bool)(availability & TileAvailabilityFlags::CONTENT_AVAILABLE) ==
              contentShouldBeAvailable);
        }
      }
    }
  }

  SUBCASE("Test children subtree availability") {
    // Check child subtree availability, none are loaded yet.
    uint32_t componentLengthAtLevel = 1U << 3;
    for (uint32_t y = 0; y < componentLengthAtLevel; ++y) {
      for (uint32_t x = 0; x < componentLengthAtLevel; ++x) {
        QuadtreeTileID id(3, x, y);
        uint8_t availability = quadtreeAvailability.computeAvailability(id);
        std::optional<uint32_t> childIndex =
            quadtreeAvailability.findChildNodeIndex(id, pParentNode);

        bool subtreeShouldBeAvailable = true;
        for (const QuadtreeTileID& tileID : unavailableSubtreeIds) {
          if (tileID == id) {
            subtreeShouldBeAvailable = false;
            break;
          }
        }

        REQUIRE(
            (bool)(availability & TileAvailabilityFlags::SUBTREE_AVAILABLE) ==
            subtreeShouldBeAvailable);

        REQUIRE((childIndex != std::nullopt) == subtreeShouldBeAvailable);
      }
    }
  }

  SUBCASE("Test children subtree loaded flag") {
    // Mock loaded child subtrees for tile IDs (3, 0, 0), (3, 0, 1), (3, 0, 2),
    // and (3, 1, 2).
    QuadtreeTileID mockChildrenSubtreeIds[]{
        QuadtreeTileID(3, 0, 0),
        QuadtreeTileID(3, 0, 1),
        QuadtreeTileID(3, 0, 2),
        QuadtreeTileID(3, 1, 2)};

    SUBCASE("Use addSubtree(id, newSubtree)") {
      for (const QuadtreeTileID& mockChildrenSubtreeId :
           mockChildrenSubtreeIds) {
        AvailabilitySubtree childSubtree{
            ConstantAvailability{true},
            ConstantAvailability{true},
            ConstantAvailability{false},
            {}};

        quadtreeAvailability.addSubtree(
            mockChildrenSubtreeId,
            std::move(childSubtree));
      }
    }

    SUBCASE("Use addNode and addLoadedSubtree") {
      for (const QuadtreeTileID& mockChildrenSubtreeId :
           mockChildrenSubtreeIds) {
        AvailabilitySubtree childSubtree{
            ConstantAvailability{true},
            ConstantAvailability{true},
            ConstantAvailability{false},
            {}};

        AvailabilityNode* pNode =
            quadtreeAvailability.addNode(mockChildrenSubtreeId, pParentNode);

        REQUIRE(pNode != nullptr);
        quadtreeAvailability.addLoadedSubtree(pNode, std::move(childSubtree));
      }
    }

    // Check that the correct child subtrees are noted to be loaded.
    uint32_t componentLengthAtLevel = 1U << 3;
    for (uint32_t y = 0; y < componentLengthAtLevel; ++y) {
      for (uint32_t x = 0; x < componentLengthAtLevel; ++x) {
        QuadtreeTileID id(3, x, y);

        // Test computeAvailability
        uint8_t availability = quadtreeAvailability.computeAvailability(id);

        // Test findChildNode
        AvailabilityNode* pChildNode =
            quadtreeAvailability.findChildNode(id, pParentNode);

        bool subtreeShouldBeLoaded = false;
        for (const QuadtreeTileID& tileID : mockChildrenSubtreeIds) {
          if (tileID == id) {
            subtreeShouldBeLoaded = true;
            break;
          }
        }

        REQUIRE(
            (bool)(availability & TileAvailabilityFlags::SUBTREE_LOADED) ==
            subtreeShouldBeLoaded);

        REQUIRE((pChildNode != nullptr) == subtreeShouldBeLoaded);
      }
    }
  }
}
