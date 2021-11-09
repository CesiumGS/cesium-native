#include "CesiumGeospatial/S2CellID.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127 4996)

#define _CHAR_UNSIGNED
#define NOMINMAX
#define _USE_MATH_DEFINES
#endif

#include <s2/s2cell_id.h>
#include <s2/s2latlng.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

using GoogleS2CellID = S2CellId;

/*static*/ S2CellID S2CellID::fromToken(const std::string_view& token) {
  return S2CellID(GoogleS2CellID::FromToken(token.data(), token.size()).id());
}

/*static*/ S2CellID S2CellID::fromQuadtreeTileID(
    uint8_t face,
    const QuadtreeTileID& quadtreeTileID) {

  // TODO: check if understanding is right here!

  uint64_t id = 0x1000000000000000UL;

  for (int32_t i = static_cast<int32_t>(quadtreeTileID.level); i >= 0; --i) {
    const uint32_t bitmask = static_cast<uint32_t>(1 << i);
    id >>= 2;

    if ((quadtreeTileID.x & bitmask) != 0) {
      id |= 0x0800000000000000UL;
    }

    if ((quadtreeTileID.y & bitmask) != 0) {
      id |= 0x1000000000000000UL;
    }
  }

  id |= (uint64_t)face << 60;

  return S2CellID(id);
}

S2CellID::S2CellID(uint64_t id) : _id(id) {}

bool S2CellID::isValid() const { return GoogleS2CellID(this->_id).is_valid(); }

std::string S2CellID::toToken() const {
  return GoogleS2CellID(this->_id).ToToken();
}

int32_t S2CellID::getLevel() const { return GoogleS2CellID(this->_id).level(); }

uint8_t S2CellID::getFace() const {
  return static_cast<uint8_t>(GoogleS2CellID(this->_id).face());
}

Cartographic S2CellID::getCenter() const {
  S2LatLng ll = GoogleS2CellID(this->_id).ToLatLng();
  return Cartographic(ll.lng().radians(), ll.lat().radians(), 0.0);
}

namespace {
Cartographic toCartographic(const S2Point& p) {
  S2LatLng ll(p);
  return Cartographic(ll.lng().radians(), ll.lat().radians(), 0.0);
}
} // namespace

std::array<Cartographic, 4> S2CellID::getVertices() const {
  GoogleS2CellID cell(this->_id);
  R2Rect rect = cell.GetBoundUV();
  int face = cell.face();
  return {
      toCartographic(S2::FaceUVtoXYZ(face, rect.GetVertex(0, 0))),
      toCartographic(S2::FaceUVtoXYZ(face, rect.GetVertex(1, 0))),
      toCartographic(S2::FaceUVtoXYZ(face, rect.GetVertex(1, 1))),
      toCartographic(S2::FaceUVtoXYZ(face, rect.GetVertex(0, 1)))};
}
