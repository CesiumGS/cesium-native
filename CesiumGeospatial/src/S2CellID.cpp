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

using namespace CesiumGeospatial;

using GoogleS2CellID = S2CellId;

/*static*/ S2CellID S2CellID::fromToken(const std::string_view& token) {
  return S2CellID(GoogleS2CellID::FromToken(token.data(), token.size()).id());
}

S2CellID::S2CellID(uint64_t id) : _id(id) {}

bool S2CellID::isValid() const {
  return GoogleS2CellID(this->_id).is_valid();
}

std::string S2CellID::toToken() const {
  return GoogleS2CellID(this->_id).ToToken();
}

int32_t S2CellID::getLevel() const { return GoogleS2CellID(this->_id).level(); }

Cartographic S2CellID::getCenter() const {
  S2LatLng ll = GoogleS2CellID(this->_id).ToLatLng();
  return Cartographic(ll.lng().radians(), ll.lat().radians(), 0.0);
}

GlobeRectangle S2CellID::getVertices() const {
  GoogleS2CellID cell(this->_id);
  R2Rect rect = cell.GetBoundUV();
  S2LatLng sw(S2::FaceUVtoXYZ(cell.face(), rect.lo()));
  S2LatLng ne(S2::FaceUVtoXYZ(cell.face(), rect.hi()));
  return GlobeRectangle(
      sw.lng().radians(),
      sw.lat().radians(),
      ne.lng().radians(),
      ne.lat().radians());
}
