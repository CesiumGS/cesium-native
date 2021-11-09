#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4127 4309 4996)

#define _CHAR_UNSIGNED
#define NOMINMAX
#define _USE_MATH_DEFINES
#endif

// #include <s2/s2cell.h>
#include <s2/s2cell_id.h>
#include <s2/s2latlng.h>
// #include <s2/s2latlng_rect.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "CesiumGeospatial/S2CellID.h"

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

using GoogleS2CellID = S2CellId;
// using GoogleS2Cell = S2Cell;

/*static*/ S2CellID S2CellID::fromToken(const std::string_view& token) {
  return S2CellID(GoogleS2CellID::FromToken(token.data(), token.size()).id());
}

S2CellID::S2CellID(uint64_t id) : _id(id) {}

bool S2CellID::isValid() const { return GoogleS2CellID(this->_id).is_valid(); }

std::string S2CellID::toToken() const {
  return GoogleS2CellID(this->_id).ToToken();
}

int32_t S2CellID::getLevel() const { return GoogleS2CellID(this->_id).level(); }

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

// GlobeRectangle S2CellID::computeBoundingRectangle() const {
//   GoogleS2Cell cell(GoogleS2CellID(this->_id));
//   S2LatLngRect rect = cell.GetRectBound();
//   return GlobeRectangle(
//       rect.lng_lo().radians(),
//       rect.lat_lo().radians(),
//       rect.lng_hi().radians(),
//       rect.lat_hi().radians());
// }
