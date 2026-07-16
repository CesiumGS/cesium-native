#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/OrientedBoundingBox.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double3.hpp>

#include <cstddef>
#include <vector>

namespace CesiumGeometry {
/*static*/ AxisAlignedBox
AxisAlignedBox::fromPositions(const std::vector<glm::dvec3>& positions) {
  if (positions.size() == 0) {
    return AxisAlignedBox();
  }

  glm::dvec3 min = positions[0];
  glm::dvec3 max = positions[0];

  for (size_t i = 1; i < positions.size(); i++) {
    min = glm::min(min, positions[i]);
    max = glm::max(max, positions[i]);
  }

  return AxisAlignedBox(min.x, min.y, min.z, max.x, max.y, max.z);
}

bool intersects(const AxisAlignedBox& b0, const AxisAlignedBox& b1) {
  // Do all the axes overlap?
  return b0.minimumX <= b1.maximumX && b0.maximumX >= b1.minimumX &&
         b0.minimumY <= b1.maximumY && b0.maximumY >= b1.minimumY &&
         b0.minimumZ <= b1.maximumZ && b0.maximumZ >= b1.minimumZ;
}

OrientedBoundingBox toOrientedBoundingBox(const AxisAlignedBox& box) {
  return OrientedBoundingBox(
      box.center,
      glm::dmat3(
          {box.lengthX * .5, 0.0, 0.0},
          {0.0, box.lengthY * .5, 0.0},
          {0.0, 0.0, box.lengthZ * .5}));
}
} // namespace CesiumGeometry
