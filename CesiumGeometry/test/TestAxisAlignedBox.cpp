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

TEST_CASE("AxisAlignedBox fromPositions") {
  SUBCASE("returns default box for empty vector") {
    AxisAlignedBox aabb = AxisAlignedBox::fromPositions({});

    CHECK(aabb.minimumX == 0.0);
    CHECK(aabb.minimumY == 0.0);
    CHECK(aabb.minimumZ == 0.0);
    CHECK(aabb.maximumX == 0.0);
    CHECK(aabb.maximumY == 0.0);
    CHECK(aabb.maximumZ == 0.0);
  }

  SUBCASE("works for single position") {
    glm::dvec3 position(1.0, 2.0, 3.0);
    AxisAlignedBox aabb = AxisAlignedBox::fromPositions({position});

    CHECK(aabb.minimumX == position.x);
    CHECK(aabb.minimumY == position.y);
    CHECK(aabb.minimumZ == position.z);
    CHECK(aabb.maximumX == position.x);
    CHECK(aabb.maximumY == position.y);
    CHECK(aabb.maximumZ == position.z);
  }

  SUBCASE("works for multiple positions") {
    std::vector<glm::dvec3> positions{
        glm::dvec3(1.0, 2.0, 3.0),
        glm::dvec3(-2.0, 0.4, -10.0),
        glm::dvec3(0.1, 4.3, 11.0),
        glm::dvec3(0.5, 0.5, 2.7)};

    AxisAlignedBox aabb = AxisAlignedBox::fromPositions(positions);
    CHECK(aabb.minimumX == -2.0);
    CHECK(aabb.minimumY == 0.4);
    CHECK(aabb.minimumZ == -10.0);
    CHECK(aabb.maximumX == 1.0);
    CHECK(aabb.maximumY == 4.3);
    CHECK(aabb.maximumZ == 11.0);
  }
}
