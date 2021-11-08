#pragma once

#include "Cartographic.h"
#include "GlobeRectangle.h"
#include "Library.h"

#include <cstdint>
#include <string_view>

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
  static S2CellID fromToken(const std::string_view& token);

  S2CellID(uint64_t id);

  bool isValid() const;
  uint64_t getID() const { return this->_id; }
  std::string toToken() const;

  int32_t getLevel() const;
  Cartographic getCenter() const;
  GlobeRectangle getVertices() const;

private:
  uint64_t _id;
};

} // namespace CesiumGeospatial
