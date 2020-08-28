#include "CesiumGeometry/Rectangle.h"

namespace CesiumGeometry {

    Rectangle::Rectangle(double minimumX, double minimumY, double maximumX, double maximumY) :
        minimumX(minimumX),
        minimumY(minimumY),
        maximumX(maximumX),
        maximumY(maximumY)
    {
    }

    bool Rectangle::contains(const glm::dvec2& position) const {
        return
            position.x >= this->minimumX &&
            position.y >= this->minimumY &&
            position.x <= this->maximumX &&
            position.y <= this->maximumY;
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
