#include <CesiumGeometry/AxisAlignedBox.h>

#include <doctest/doctest.h>

using namespace CesiumGeometry;

TEST_CASE("AxisAlignedBox constructor") {
  AxisAlignedBox aabb(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
  CHECK(aabb.minimumX == 1.0);
  CHECK(aabb.minimumY == 2.0);
  CHECK(aabb.minimumZ == 3.0);
  CHECK(aabb.maximumX == 4.0);
  CHECK(aabb.maximumY == 5.0);
  CHECK(aabb.maximumZ == 6.0);
  CHECK(aabb.center.x == (1.0 + 4.0) * 0.5);
  CHECK(aabb.center.y == (2.0 + 5.0) * 0.5);
  CHECK(aabb.center.z == (3.0 + 6.0) * 0.5);
  CHECK(aabb.lengthX == 4.0 - 1.0);
  CHECK(aabb.lengthY == 5.0 - 2.0);
  CHECK(aabb.lengthZ == 6.0 - 3.0);
}
