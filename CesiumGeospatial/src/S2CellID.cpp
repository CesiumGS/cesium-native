#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4127 4309 4996)

#define _CHAR_UNSIGNED
#define NOMINMAX
#define _USE_MATH_DEFINES
#endif

// #include <s2/s2cell.h>
#include <s2/r2rect.h>
#include <s2/s1interval.h>
#include <s2/s2cell_id.h>
#include <s2/s2coords.h>
#include <s2/s2latlng.h>
#include <s2/s2point.h>
// #include <s2/s2latlng_rect.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "HilbertOrder.h"

#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/S2CellID.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Math.h>

#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

using GoogleS2CellID = S2CellId;

/*static*/ S2CellID S2CellID::fromToken(const std::string_view& token) {
  return S2CellID(
      GoogleS2CellID::FromToken(absl::string_view(token.data(), token.size()))
          .id());
}

/*static*/ S2CellID S2CellID::fromFaceLevelPosition(
    uint8_t face,
    uint32_t level,
    uint64_t position) {
  // Convert the position-within-level to an absolute, level 30 position as
  // expected by the Google implementation.
  position <<= (GoogleS2CellID::kMaxLevel - level) * 2 + 1;
  return S2CellID(
      GoogleS2CellID::FromFacePosLevel(int(face), position, int(level)).id());
}

/*static*/ S2CellID S2CellID::fromQuadtreeTileID(
    uint8_t face,
    const QuadtreeTileID& quadtreeTileID) {

  uint64_t position = (face & 1) == 0 ? HilbertOrder::encode2D(
                                            quadtreeTileID.level,
                                            quadtreeTileID.x,
                                            quadtreeTileID.y)
                                      : HilbertOrder::encode2D(
                                            quadtreeTileID.level,
                                            quadtreeTileID.y,
                                            quadtreeTileID.x);

  return S2CellID::fromFaceLevelPosition(face, quadtreeTileID.level, position);
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

S2CellID S2CellID::getParent() const {
  return S2CellID(GoogleS2CellID(this->_id).parent().id());
}

S2CellID S2CellID::getChild(size_t index) const {
  CESIUM_ASSERT(index <= 3);
  return S2CellID(GoogleS2CellID(this->_id).child(int(index)).id());
}

namespace {

// These anonymous-namespace functions are adaptations from s2geometry.
double GetLatitude(const R2Rect& uv_, int face_, int i, int j) {
  S2Point p = S2::FaceUVtoXYZ(face_, uv_[0][i], uv_[1][j]);
  return S2LatLng::Latitude(p).radians();
}

double GetLongitude(const R2Rect& uv_, int face_, int i, int j) {
  S2Point p = S2::FaceUVtoXYZ(face_, uv_[0][i], uv_[1][j]);
  return S2LatLng::Longitude(p).radians();
}

GlobeRectangle
GlobeRectangleFromLatLng(const R1Interval& lat_, const S1Interval& lng_) {
  return GlobeRectangle(lng_.lo(), lat_.lo(), lng_.hi(), lat_.hi());
}

R1Interval FullLat() {
  return R1Interval(
      -CesiumUtility::Math::PiOverTwo,
      CesiumUtility::Math::PiOverTwo);
}

GlobeRectangle Expanded(
    const R1Interval& lat_,
    const S1Interval& lng_,
    const S2LatLng& margin) {
  R1Interval lat = lat_.Expanded(margin.lat().radians());
  S1Interval lng = lng_.Expanded(margin.lng().radians());
  if (lat.is_empty() || lng.is_empty())
    return GlobeRectangle(0.0, 0.0, 0.0, 0.0);
  return GlobeRectangleFromLatLng(lat.Intersection(FullLat()), lng);
}

} // namespace

GlobeRectangle S2CellID::computeBoundingRectangle() const {
  GoogleS2CellID google(this->_id);
  R2Rect uv_ = google.GetBoundUV();
  int level_ = google.level();
  int face_ = google.face();

  // The below code is copied from s2geometry's s2cell.cc.
  // We copy it rather than just call it like we do with other code in that
  // library because s2cell.cc has some steep dependencies that eventually lead
  // to requiring OpenSSL.

  // However, we've changed the implementation slightly from s2geometry. In
  // s2geometry, a cell that contains the North or South pole is deemed to
  // include all longitudes. While this is technically true, because any
  // longitude at +/-PI/2 radians latitude is at the pole, it is still
  // "losing information" to present it that way. We'd rather return the
  // actual longitude bounds and let client code account for the singularity
  // when necessary. So we don't do the "PolarClosure" operation performed by
  // the original implementation.

  if (level_ > 0) {
    // Except for cells at level 0, the latitude and longitude extremes are
    // attained at the vertices.  Furthermore, the latitude range is
    // determined by one pair of diagonally opposite vertices and the
    // longitude range is determined by the other pair.
    //
    // We first determine which corner (i,j) of the cell has the largest
    // absolute latitude.  To maximize latitude, we want to find the point in
    // the cell that has the largest absolute z-coordinate and the smallest
    // absolute x- and y-coordinates.  To do this we look at each coordinate
    // (u and v), and determine whether we want to minimize or maximize that
    // coordinate based on the axis direction and the cell's (u,v) quadrant.
    double u = uv_[0][0] + uv_[0][1];
    double v = uv_[1][0] + uv_[1][1];
    int i = S2::GetUAxis(face_)[2] == 0 ? (u < 0) : (u > 0);
    int j = S2::GetVAxis(face_)[2] == 0 ? (v < 0) : (v > 0);
    R1Interval lat = R1Interval::FromPointPair(
        GetLatitude(uv_, face_, i, j),
        GetLatitude(uv_, face_, 1 - i, 1 - j));
    S1Interval lng = S1Interval::FromPointPair(
        GetLongitude(uv_, face_, i, 1 - j),
        GetLongitude(uv_, face_, 1 - i, j));

    // We grow the bounds slightly to make sure that the bounding rectangle
    // contains S2LatLng(P) for any point P inside the loop L defined by the
    // four *normalized* vertices.  Note that normalization of a vector can
    // change its direction by up to 0.5 * DBL_EPSILON radians, and it is not
    // enough just to add Normalize() calls to the code above because the
    // latitude/longitude ranges are not necessarily determined by diagonally
    // opposite vertex pairs after normalization.
    //
    // We would like to bound the amount by which the latitude/longitude of a
    // contained point P can exceed the bounds computed above.  In the case of
    // longitude, the normalization error can change the direction of rounding
    // leading to a maximum difference in longitude of 2 * DBL_EPSILON.  In
    // the case of latitude, the normalization error can shift the latitude by
    // up to 0.5 * DBL_EPSILON and the other sources of error can cause the
    // two latitudes to differ by up to another 1.5 * DBL_EPSILON, which also
    // leads to a maximum difference of 2 * DBL_EPSILON.
    return Expanded(
        lat,
        lng,
        S2LatLng::FromRadians(2 * DBL_EPSILON, 2 * DBL_EPSILON));
  }

  // The 4 cells around the equator extend to +/-45 degrees latitude at the
  // midpoints of their top and bottom edges.  The two cells covering the
  // poles extend down to +/-35.26 degrees at their vertices.  The maximum
  // error in this calculation is 0.5 * DBL_EPSILON.
  static const double kPoleMinLat = asin(sqrt(1. / 3)) - 0.5 * DBL_EPSILON;

  // The face centers are the +X, +Y, +Z, -X, -Y, -Z axes in that order.
  CESIUM_ASSERT(((face_ < 3) ? 1 : -1) == S2::GetNorm(face_)[face_ % 3]);

  R1Interval lat;
  S1Interval lng;
  switch (face_) {
  case 0:
    lat = R1Interval(
        -CesiumUtility::Math::PiOverFour,
        CesiumUtility::Math::PiOverFour);
    lng = S1Interval(
        -CesiumUtility::Math::PiOverFour,
        CesiumUtility::Math::PiOverFour);
    break;
  case 1:
    lat = R1Interval(
        -CesiumUtility::Math::PiOverFour,
        CesiumUtility::Math::PiOverFour);
    lng = S1Interval(
        CesiumUtility::Math::PiOverFour,
        3 * CesiumUtility::Math::PiOverFour);
    break;
  case 2:
    lat = R1Interval(kPoleMinLat, CesiumUtility::Math::PiOverTwo);
    lng = S1Interval::Full();
    break;
  case 3:
    lat = R1Interval(
        -CesiumUtility::Math::PiOverFour,
        CesiumUtility::Math::PiOverFour);
    lng = S1Interval(
        3 * CesiumUtility::Math::PiOverFour,
        -3 * CesiumUtility::Math::PiOverFour);
    break;
  case 4:
    lat = R1Interval(
        -CesiumUtility::Math::PiOverFour,
        CesiumUtility::Math::PiOverFour);
    lng = S1Interval(
        -3 * CesiumUtility::Math::PiOverFour,
        -CesiumUtility::Math::PiOverFour);
    break;
  default:
    lat = R1Interval(-CesiumUtility::Math::PiOverTwo, -kPoleMinLat);
    lng = S1Interval::Full();
    break;
  }
  // Finally, we expand the bound to account for the error when a point P is
  // converted to an S2LatLng to test for containment.  (The bound should be
  // large enough so that it contains the computed S2LatLng of any contained
  // point, not just the infinite-precision version.)  We don't need to expand
  // longitude because longitude is calculated via a single call to atan2(),
  // which is guaranteed to be semi-monotonic.  (In fact the Gnu implementation
  // is also correctly rounded, but we don't even need that here.)
  return Expanded(lat, lng, S2LatLng::FromRadians(DBL_EPSILON, 0));
}
