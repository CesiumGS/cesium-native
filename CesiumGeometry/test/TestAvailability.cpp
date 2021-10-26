
#include "CesiumGeometry/Availability.h"
#include "CesiumGeometry/OctreeAvailability.h"
#include "CesiumGeometry/QuadtreeAvailability.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumGeometry/TileAvailabilityFlags.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <algorithm>
#include <memory>
#include <vector>

using namespace CesiumGeometry;

TEST_CASE("Test AvailabilityUtilities") {
  SECTION("Test countOnesInByte") {
    uint8_t byte = static_cast<uint8_t>(0xFF);
    for (uint8_t i = 0; i <= 8; ++i) {
      REQUIRE(
          AvailabilityUtilities::countOnesInByte(
              static_cast<uint8_t>(byte >> i)) == (8 - i));
    }
  }

  SECTION("Test countOnesInBuffer") {
    std::vector<std::byte> buffer(64);
    for (size_t i = 0; i < 64U; ++i) {
      buffer[i] = static_cast<std::byte>(0xFC);
    }

    // Each byte is 0xFC which has 6 ones.
    // This means there are 6 x 64 = 384 ones total in the buffer.
    uint32_t onesInBuffer = AvailabilityUtilities::countOnesInBuffer(
        gsl::span<std::byte>(&buffer[0], 64));
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

  SECTION("Test constant availability") {
    REQUIRE(tileAvailabilityAccessor.isConstant());
    REQUIRE(tileAvailabilityAccessor.getConstant());
    REQUIRE(!tileAvailabilityAccessor.isBufferView());
    REQUIRE(subtreeAvailabilityAccessor.isConstant());
    REQUIRE(!subtreeAvailabilityAccessor.getConstant());
    REQUIRE(!subtreeAvailabilityAccessor.isBufferView());
  }

  SECTION("Test buffer availability") {
    REQUIRE(!contentAvailabilityAccessor.isConstant());
    REQUIRE(contentAvailabilityAccessor.isBufferView());
    REQUIRE(contentAvailabilityAccessor.size() == 64);
    for (size_t i = 0; i < 64U; ++i) {
      REQUIRE(contentAvailabilityAccessor[i] == static_cast<std::byte>(0xFC));
    }
  }

  SECTION("Test combined buffer availability") {
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

TEST_CASE("Test QuadtreeAvailability") {
  // We will test with a quadtree availability subtree with 3 levels.

  // The tile and content availability will require bitstreams with 21 bits,
  // but we will need to byte-align to 8 bytes minimum for each.

  // The subtree availability bitstream will require 64 bits (exactly 8 bytes).

  // All tiles in the subtree will have tile availability.
  std::vector<std::byte> tileAvailabilityBuffer = {
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xfc),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00)};

  // Tiles with morton index 12, 13, 14, and 15 will not have content.
  // These are tiles 7, 8, 9, 10 in level 2.
  // These are tile IDs (2, 3, 1), (2, 0, 2), (2, 1, 2), and (2, 0, 3).
  std::vector<std::byte> contentAvailabilityBuffer = {
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xf0),
      static_cast<std::byte>(0xfc),
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
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xf0),
      static_cast<std::byte>(0xff),
      static_cast<std::byte>(0xff)};

  QuadtreeTileID unavailableSubtreeIds[]{
      QuadtreeTileID(3, 2, 6),
      QuadtreeTileID(3, 3, 6),
      QuadtreeTileID(3, 2, 7),
      QuadtreeTileID(3, 3, 7)};

  AvailabilitySubtree subtree{
      SubtreeBufferView{0, 8, 0},
      SubtreeBufferView{0, 8, 1},
      SubtreeBufferView{0, 8, 2},
      {tileAvailabilityBuffer,
       contentAvailabilityBuffer,
       subtreeAvailabilityBuffer}};

  QuadtreeAvailability quadtreeAvailability(
      QuadtreeTilingScheme(Rectangle(0.0, 0.0, 1.0, 1.0), 1, 1),
      3,
      5);
  quadtreeAvailability.addSubtree(QuadtreeTileID(0, 0, 0), std::move(subtree));

  SECTION("Test tile and content availability") {
    for (uint32_t level = 0; level < 3U; ++level) {
      for (uint32_t y = 0; y < (1U << level); ++y) {
        for (uint32_t x = 0; x < (1U << level); ++x) {
          QuadtreeTileID id(level, x, y);
          uint8_t availability = quadtreeAvailability.computeAvailability(id);

          // All tiles should be available.
          REQUIRE(availability & TileAvailabilityFlags::TILE_AVAILABLE);

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

  SECTION("Test children subtree availability") {
    // Check child subtree availability, none are loaded yet.
    uint32_t componentLengthAtLevel = 1U << 3;
    for (uint32_t y = 0; y < componentLengthAtLevel; ++y) {
      for (uint32_t x = 0; x < componentLengthAtLevel; ++x) {
        QuadtreeTileID id(3, x, y);
        uint8_t availability = quadtreeAvailability.computeAvailability(id);

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
      }
    }
  }

  SECTION("Test children subtree loaded flag") {
    // Mock loaded child subtrees for tile IDs (3, 0, 0), (3, 0, 1), (3, 0, 2),
    // and (3, 1, 2).
    QuadtreeTileID mockChildrenSubtreeIds[]{
        QuadtreeTileID(3, 0, 0),
        QuadtreeTileID(3, 0, 1),
        QuadtreeTileID(3, 0, 2),
        QuadtreeTileID(3, 1, 2)};

    for (const QuadtreeTileID& mockChildrenSubtreeId : mockChildrenSubtreeIds) {
      AvailabilitySubtree childSubtree{
          ConstantAvailability{true},
          ConstantAvailability{true},
          ConstantAvailability{false},
          {}};

      quadtreeAvailability.addSubtree(
          mockChildrenSubtreeId,
          std::move(childSubtree));
    }

    // Check that the correct child subtrees are noted to be loaded.
    uint32_t componentLengthAtLevel = 1U << 3;
    for (uint32_t y = 0; y < componentLengthAtLevel; ++y) {
      for (uint32_t x = 0; x < componentLengthAtLevel; ++x) {
        QuadtreeTileID id(3, x, y);
        uint8_t availability = quadtreeAvailability.computeAvailability(id);

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
      }
    }
  }
}