#include <CesiumGeometry/AxisAlignedBox.h>

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
} // namespace CesiumGeometry
