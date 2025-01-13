#include <CesiumGeometry/Rectangle.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/geometric.hpp>

#include <optional>

namespace CesiumGeometry {

bool Rectangle::contains(const glm::dvec2& position) const noexcept {
  return position.x >= this->minimumX && position.y >= this->minimumY &&
         position.x <= this->maximumX && position.y <= this->maximumY;
}

bool Rectangle::overlaps(const Rectangle& other) const noexcept {
  const double left = glm::max(this->minimumX, other.minimumX);
  const double bottom = glm::max(this->minimumY, other.minimumY);
  const double right = glm::min(this->maximumX, other.maximumX);
  const double top = glm::min(this->maximumY, other.maximumY);
  return bottom < top && left < right;
}

bool Rectangle::fullyContains(const Rectangle& other) const noexcept {
  return (
      other.minimumX >= this->minimumX && other.maximumX <= this->maximumX &&
      other.minimumY >= this->minimumY && other.maximumY <= this->maximumY);
}

double
Rectangle::computeSignedDistance(const glm::dvec2& position) const noexcept {
  const glm::dvec2 bottomLeftDistance =
      glm::dvec2(minimumX, minimumY) - position;
  const glm::dvec2 topRightDistance = position - glm::dvec2(maximumX, maximumY);
  const glm::dvec2 maxDistance = glm::max(bottomLeftDistance, topRightDistance);

  if (maxDistance.x <= 0.0 && maxDistance.y <= 0.0) {
    // Inside, report closest edge.
    return glm::max(maxDistance.x, maxDistance.y);
  }
  if (maxDistance.x > 0.0 && maxDistance.y > 0.0) {
    // Outside in both directions, closest point is a corner
    return glm::length(maxDistance);
  }
  // Outside in one direction, report the distance in that direction.
  return glm::max(maxDistance.x, maxDistance.y);
}

std::optional<Rectangle>
Rectangle::computeIntersection(const Rectangle& other) const noexcept {
  const double left = glm::max(this->minimumX, other.minimumX);
  const double bottom = glm::max(this->minimumY, other.minimumY);
  const double right = glm::min(this->maximumX, other.maximumX);
  const double top = glm::min(this->maximumY, other.maximumY);

  if (bottom >= top || left >= right) {
    return std::nullopt;
  }

  return Rectangle(left, bottom, right, top);
}

Rectangle Rectangle::computeUnion(const Rectangle& other) const noexcept {
  return Rectangle(
      glm::min(this->minimumX, other.minimumX),
      glm::min(this->minimumY, other.minimumY),
      glm::max(this->maximumX, other.maximumX),
      glm::max(this->maximumY, other.maximumY));
}

} // namespace CesiumGeometry
