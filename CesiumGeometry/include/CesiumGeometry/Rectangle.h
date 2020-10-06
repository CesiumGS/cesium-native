#pragma once

#include "CesiumGeometry/Library.h"
#include <glm/vec2.hpp>
#include <optional>

namespace CesiumGeometry {

    class CESIUMGEOMETRY_API Rectangle {
    public:
        Rectangle(double minimumX, double minimumY, double maximumX, double maximumY);

        double minimumX;
        double minimumY;
        double maximumX;
        double maximumY;

        bool contains(const glm::dvec2& position) const;
        bool overlaps(const Rectangle& other) const;
        bool fullyContains(const Rectangle& other) const;

        glm::dvec2 getLowerLeft() const { return glm::dvec2(this->minimumX, this->minimumY); }
        glm::dvec2 getLowerRight() const { return glm::dvec2(this->maximumX, this->minimumY); }
        glm::dvec2 getUpperLeft() const { return glm::dvec2(this->minimumX, this->maximumY); }
        glm::dvec2 getUpperRight() const { return glm::dvec2(this->maximumX, this->maximumY); }
        glm::dvec2 getCenter() const { return glm::dvec2((this->minimumX + this->maximumX) * 0.5, (this->minimumY + this->maximumY) * 0.5); }

        double computeWidth() const;
        double computeHeight() const;

        /**
         * Computes the intersection of this rectangle with another.
         *
         * @param other The other rectangle to intersect with this one.
         * @returns The intersection rectangle, or `std::nullopt` if there is no intersection.
         */
        std::optional<Rectangle> intersect(const Rectangle& other) const;
    };

}
