#pragma once

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Library.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace CesiumGeometry {
struct QuadtreeTileID;
}

namespace CesiumGeospatial {

/**
 * @brief A 64-bit unsigned integer that uniquely identifies a
 * cell in the S2 cell decomposition.
 *
 * It has the following format:
 *
 *   id = [face][face_pos]
 *
 *   face:     a 3-bit number (range 0..5) encoding the cube face.
 *
 *   face_pos: a 61-bit number encoding the position of the center of this
 *             cell along the Hilbert curve over this face (see the Wiki
 *             pages for details).
 *
 * Sequentially increasing cell ids follow a continuous space-filling curve
 * over the entire sphere.  They have the following properties:
 *
 *  - The id of a cell at level k consists of a 3-bit face number followed
 *    by k bit pairs that recursively select one of the four children of
 *    each cell.  The next bit is always 1, and all other bits are 0.
 *    Therefore, the level of a cell is determined by the position of its
 *    lowest-numbered bit that is turned on (for a cell at level k, this
 *    position is 2 * (kMaxLevel - k).)
 *
 *  - The id of a parent cell is at the midpoint of the range of ids spanned
 *    by its children (or by its descendants at any level).
 *
 * This class is adapted from S2CellId in https://github.com/google/s2geometry.
 */
class CESIUMGEOSPATIAL_API S2CellID {
public:
  /**
   * Creates a new S2Cell from a token. A token is a hexadecimal representation
   * of the 64-bit S2CellID.
   *
   * @param token The token for the S2 Cell.
   * @returns A new S2CellID.
   */
  static S2CellID fromToken(const std::string_view& token);

  /**
   * @brief Creates a cell given its face (range 0..5), level, and Hilbert curve
   * cell index within that face and level.
   *
   * @param face The face index.
   * @param level The level within the face.
   * @param position The Hilbert-order index of the cell within the face and
   * level.
   * @return The cell.
   */
  static S2CellID
  fromFaceLevelPosition(uint8_t face, uint32_t level, uint64_t position);

  /**
   * @brief Create an S2 id from a face and a quadtree tile id.
   *
   * @param face The S2 face (0...5) that this tile is on.
   * @param quadtreeTileID The quadtree tile id for this tile, within the given
   * face.
   */
  static S2CellID fromQuadtreeTileID(
      uint8_t face,
      const CesiumGeometry::QuadtreeTileID& quadtreeTileID);

  /**
   * @brief Constructs a new S2 cell ID.
   *
   * The cell ID value is not validated. Use {@link isValid} to check the
   * validity after constructions.
   *
   * @param id The 64-bit cell ID value.
   */
  S2CellID(uint64_t id);

  /**
   * @brief Determines if this cell ID is valid.
   *
   * @return true if the the cell ID refers to a valid cell; otherwise, false.
   */
  bool isValid() const;

  /**
   * @brief Gets the ID of the cell.
   *
   * @return The ID.
   */
  uint64_t getID() const { return this->_id; }

  /**
   * @brief Converts the cell ID to a hexadecimal token.
   */
  std::string toToken() const;

  /**
   * @brief Gets the level of the cell from the cell ID.
   *
   * @return The cell ID, where 0 is the root.
   */
  int32_t getLevel() const;

  /**
   * @brief Gets the face id (0...5) for this S2 cell.
   */
  uint8_t getFace() const;

  /**
   * @brief Gets the longitude/latitude position at the center of this cell.
   *
   * The height is always 0.0.
   *
   * @return The center.
   */
  Cartographic getCenter() const;

  /**
   * @brief Gets the vertices at the corners of the cell.
   *
   * The height values are always 0.0.
   *
   * Note that all positions inside the S2 Cell are _not_ guaranteed to fall
   * inside the rectangle formed by these vertices.
   *
   * @return Four vertices specifying the corners of this cell.
   */
  std::array<Cartographic, 4> getVertices() const;

  /**
   * @brief Gets the parent cell of this cell.
   *
   * If this is a root cell, the behavior is unspecified.
   */
  S2CellID getParent() const;

  /**
   * @brief Gets a child cell of this cell.
   *
   * If the index is less than 0 or greater than 3, or if this is a leaf cell,
   * the behavior is unspecified.
   *
   * @param index The index in the range 0 to 3.
   * @return The child cell.
   */
  S2CellID getChild(size_t index) const;

  /**
   * @brief Computes the globe rectangle that bounds this cell.
   *
   * @return The globe rectangle.
   */
  GlobeRectangle computeBoundingRectangle() const;

private:
  uint64_t _id;
};

} // namespace CesiumGeospatial
