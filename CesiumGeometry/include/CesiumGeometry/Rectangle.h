#pragma once

#include <CesiumGeometry/Library.h>

#include <glm/vec2.hpp>

#include <optional>

namespace CesiumGeometry {

/**
 * @brief A 2D rectangle
 */
struct CESIUMGEOMETRY_API Rectangle final {
  /**
   * @brief Creates a new instance with all coordinate values set to 0.0.
   */
  constexpr Rectangle() noexcept
      : minimumX(0.0), minimumY(0.0), maximumX(0.0), maximumY(0.0) {}

  /**
   * @brief Creates a new instance.
   *
   * Creates a new rectangle from the given coordinates. This implicitly
   * assumes that the given coordinates form a valid rectangle, meaning
   * that `minimumX <= maximumX` and `minimumY <= maximumY`.
   *
   * @param minimumX_ The minimum x-coordinate.
   * @param minimumY_ The minimum y-coordinate.
   * @param maximumX_ The maximum x-coordinate.
   * @param maximumY_ The maximum y-coordinate.
   */
  constexpr Rectangle(
      double minimumX_,
      double minimumY_,
      double maximumX_,
      double maximumY_) noexcept
      : minimumX(minimumX_),
        minimumY(minimumY_),
        maximumX(maximumX_),
        maximumY(maximumY_) {}

  /**
   * @brief The minimum x-coordinate.
   */
  double minimumX;

  /**
   * @brief The minimum y-coordinate.
   */
  double minimumY;

  /**
   * @brief The maximum x-coordinate.
   */
  double maximumX;

  /**
   * @brief The maximum y-coordinate.
   */
  double maximumY;

  /**
   * @brief Checks whether this rectangle contains the given position.
   *
   * This means that the `x`- and `y` coordinates of the given position
   * are not smaller than the minimum and not larger than the maximum
   * coordinates of this rectangle.
   *
   * @param position The position.
   * @returns Whether this rectangle contains the given position.
   */
  bool contains(const glm::dvec2& position) const noexcept;

  /**
   * @brief Checks whether this rectangle overlaps the given rectangle.
   *
   * This means that this rectangle and the given rectangle have
   * a non-empty intersection. If either of the rectangles is empty,
   * then this will always return `false`.
   *
   * @param other The other rectangle.
   * @returns Whether this rectangle overlaps the given rectangle.
   */
  bool overlaps(const Rectangle& other) const noexcept;

  /**
   * @brief Checks whether this rectangle fully contains the given rectangle.
   *
   * This means that this rectangle contains all four corner points
   * of the given rectangle, as defined in {@link Rectangle::contains}.
   *
   * @param other The other rectangle.
   * @returns Whether this rectangle fully contains the given rectangle.
   */
  bool fullyContains(const Rectangle& other) const noexcept;

  /**
   * @brief Computes the signed distance from a position to the edge of the
   * rectangle.
   *
   * If the position is inside the rectangle, the distance is negative. If it is
   * outside the rectangle, it is positive.
   *
   * @param position The position.
   * @return The signed distance.
   */
  double computeSignedDistance(const glm::dvec2& position) const noexcept;

  /**
   * @brief Returns a point at the lower left of this rectangle.
   *
   * This is the point that consists of the minimum x- and y-coordinate.
   *
   * @returns The lower left point.
   */
  constexpr glm::dvec2 getLowerLeft() const noexcept {
    return glm::dvec2(this->minimumX, this->minimumY);
  }

  /**
   * @brief Returns a point at the lower right of this rectangle.
   *
   * This is the point that consists of the maximum x- and minimum y-coordinate.
   *
   * @returns The lower right point.
   */
  constexpr glm::dvec2 getLowerRight() const noexcept {
    return glm::dvec2(this->maximumX, this->minimumY);
  }

  /**
   * @brief Returns a point at the upper left of this rectangle.
   *
   * This is the point that consists of the minimum x- and maximum y-coordinate.
   *
   * @returns The upper left point.
   */
  constexpr glm::dvec2 getUpperLeft() const noexcept {
    return glm::dvec2(this->minimumX, this->maximumY);
  }

  /**
   * @brief Returns a point at the upper right of this rectangle.
   *
   * This is the point that consists of the maximum x- and y-coordinate.
   *
   * @returns The upper right point.
   */
  constexpr glm::dvec2 getUpperRight() const noexcept {
    return glm::dvec2(this->maximumX, this->maximumY);
  }

  /**
   * @brief Returns a point at the center of this rectangle.
   *
   * @returns The center point.
   */
  constexpr glm::dvec2 getCenter() const noexcept {
    return glm::dvec2(
        (this->minimumX + this->maximumX) * 0.5,
        (this->minimumY + this->maximumY) * 0.5);
  }

  /**
   * @brief Computes the width of this rectangle.
   *
   * @returns The width.
   */
  constexpr double computeWidth() const noexcept {
    return this->maximumX - this->minimumX;
  }

  /**
   * @brief Computes the height of this rectangle.
   *
   * @returns The height.
   */
  constexpr double computeHeight() const noexcept {
    return this->maximumY - this->minimumY;
  }

  /**
   * Computes the intersection of this rectangle with another.
   *
   * @param other The other rectangle to intersect with this one.
   * @returns The intersection rectangle, or `std::nullopt` if there is no
   * intersection.
   */
  std::optional<Rectangle>
  computeIntersection(const Rectangle& other) const noexcept;

  /**
   * @brief Computes the union of this rectangle with another.
   *
   * @param other The other rectangle to union with this one.
   * @return The union rectangle, which fully contains both rectangles.
   */
  Rectangle computeUnion(const Rectangle& other) const noexcept;
};

} // namespace CesiumGeometry
