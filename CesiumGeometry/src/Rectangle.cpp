#include "CesiumGeometry/Rectangle.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <algorithm>

namespace CesiumGeometry {

    Rectangle::Rectangle(double minimumXIn, double minimumYIn, double maximumXIn, double maximumYIn) :
        minimumX(minimumXIn),
        minimumY(minimumYIn),
        maximumX(maximumXIn),
        maximumY(maximumYIn)
    {
    }

    bool Rectangle::contains(const glm::dvec2& position) const {
        return
            position.x >= this->minimumX &&
            position.y >= this->minimumY &&
            position.x <= this->maximumX &&
            position.y <= this->maximumY;
    }

    bool Rectangle::overlaps(const Rectangle& other) const {
        double left = std::max(this->minimumX, other.minimumX);
        double bottom = std::max(this->minimumY, other.minimumY);
        double right = std::min(this->maximumX, other.maximumX);
        double top = std::min(this->maximumY, other.maximumY);
        return bottom < top && left < right;
    }

    bool Rectangle::fullyContains(const Rectangle& other) const {
        return (
            other.minimumX >= this->minimumX &&
            other.maximumX <= this->maximumX &&
            other.minimumY >= this->minimumY &&
            other.maximumY <= this->maximumY
        );
    }

    double Rectangle::computeSignedDistance(const glm::dvec2& position) const {
        glm::dvec2 bottomLeftDistance = glm::dvec2(minimumX, minimumY) - position;
        glm::dvec2 topRightDistance = position - glm::dvec2(maximumX, maximumY);
        glm::dvec2 maxDistance = glm::max(bottomLeftDistance, topRightDistance);

        if (maxDistance.x <= 0.0 && maxDistance.y <= 0.0) {
            // Inside, report closest edge.
            return std::max(maxDistance.x, maxDistance.y);
        } else if (maxDistance.x > 0.0 && maxDistance.y > 0.0) {
            // Outside in both directions, closest point is a corner
            return glm::length(maxDistance);
        } else {
            // Outside in one direction, report the distance in that direction.
            return std::max(maxDistance.x, maxDistance.y);
        }
    }

    double Rectangle::computeWidth() const {
        return this->maximumX - this->minimumX;
    }

    double Rectangle::computeHeight() const {
        return this->maximumY - this->minimumY;
    }

    std::optional<Rectangle> Rectangle::intersect(const Rectangle& other) const {
        double left = std::max(this->minimumX, other.minimumX);
        double bottom = std::max(this->minimumY, other.minimumY);
        double right = std::min(this->maximumX, other.maximumX);
        double top = std::min(this->maximumY, other.maximumY);

        if (bottom >= top || left >= right) {
            return std::nullopt;
        }

        return Rectangle(left, bottom, right, top);
    }

}
