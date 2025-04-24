#pragma once

#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Library.h>
#include <CesiumUtility/Math.h>

#include <optional>

namespace CesiumGeospatial {

/**
 * @brief A two-dimensional, rectangular region on a globe, specified using
 * longitude and latitude coordinates. The region is rectangular in terms of
 * longitude-latitude coordinates, but may be far from rectangular on the actual
 * globe surface.
 *
 * The eastern coordinate may be less than the western coordinate, which
 * indicates that the rectangle crosses the anti-meridian.
 *
 * @see CesiumGeometry::Rectangle
 */
class CESIUMGEOSPATIAL_API GlobeRectangle final {
public:
  /**
   * @brief An empty rectangle.
   *
   * The rectangle has the following values:
   *   * `west`: Pi
   *   * `south`: Pi/2
   *   * `east`: -Pi
   *   * `north`: -Pi/2
   */
  static const GlobeRectangle EMPTY;

  /**
   * @brief The maximum rectangle.
   *
   * The rectangle has the following values:
   *   * `west`: -Pi
   *   * `south`: -Pi/2
   *   * `east`: Pi
   *   * `north`: Pi/2
   */
  static const GlobeRectangle MAXIMUM;

  /**
   * @brief Constructs a new instance.
   *
   * @param west The westernmost longitude, in radians, in the range [-Pi, Pi].
   * @param south The southernmost latitude, in radians, in the range [-Pi/2,
   * Pi/2].
   * @param east The easternmost longitude, in radians, in the range [-Pi, Pi].
   * @param north The northernmost latitude, in radians, in the range [-Pi/2,
   * Pi/2].
   */
  constexpr GlobeRectangle(
      double west,
      double south,
      double east,
      double north) noexcept
      : _west(west), _south(south), _east(east), _north(north) {}

  /**
   * Creates a rectangle given the boundary longitude and latitude in degrees.
   * The angles are converted to radians.
   *
   * @param westDegrees The westernmost longitude in degrees in the range
   * [-180.0, 180.0].
   * @param southDegrees The southernmost latitude in degrees in the range
   * [-90.0, 90.0].
   * @param eastDegrees The easternmost longitude in degrees in the range
   * [-180.0, 180.0].
   * @param northDegrees The northernmost latitude in degrees in the range
   * [-90.0, 90.0].
   * @returns The rectangle.
   *
   * @snippet TestGlobeRectangle.cpp fromDegrees
   */
  static constexpr GlobeRectangle fromDegrees(
      double westDegrees,
      double southDegrees,
      double eastDegrees,
      double northDegrees) noexcept {
    return GlobeRectangle(
        CesiumUtility::Math::degreesToRadians(westDegrees),
        CesiumUtility::Math::degreesToRadians(southDegrees),
        CesiumUtility::Math::degreesToRadians(eastDegrees),
        CesiumUtility::Math::degreesToRadians(northDegrees));
  }

  /**
   * @brief Returns the westernmost longitude, in radians.
   */
  constexpr double getWest() const noexcept { return this->_west; }

  /**
   * @brief Sets the westernmost longitude, in radians.
   */
  void setWest(double value) noexcept { this->_west = value; }

  /**
   * @brief Returns the southernmost latitude, in radians.
   */
  constexpr double getSouth() const noexcept { return this->_south; }

  /**
   * @brief Sets the southernmost latitude, in radians.
   */
  void setSouth(double value) noexcept { this->_south = value; }

  /**
   * @brief Returns the easternmost longitude, in radians.
   */
  constexpr double getEast() const noexcept { return this->_east; }

  /**
   * @brief Sets the easternmost longitude, in radians.
   */
  void setEast(double value) noexcept { this->_east = value; }

  /**
   * @brief Returns the northernmost latitude, in radians.
   */
  constexpr double getNorth() const noexcept { return this->_north; }

  /**
   * @brief Sets the northernmost latitude, in radians.
   */
  void setNorth(double value) noexcept { this->_north = value; }

  /**
   * @brief Returns the {@link Cartographic} position of the south-west corner.
   */
  constexpr Cartographic getSouthwest() const noexcept {
    return Cartographic(this->_west, this->_south);
  }

  /**
   * @brief Returns the {@link Cartographic} position of the south-east corner.
   */
  constexpr Cartographic getSoutheast() const noexcept {
    return Cartographic(this->_east, this->_south);
  }

  /**
   * @brief Returns the {@link Cartographic} position of the north-west corner.
   */
  constexpr Cartographic getNorthwest() const noexcept {
    return Cartographic(this->_west, this->_north);
  }

  /**
   * @brief Returns the {@link Cartographic} position of the north-east corner.
   */
  constexpr Cartographic getNortheast() const noexcept {
    return Cartographic(this->_east, this->_north);
  }

  /**
   * @brief Returns this rectangle as a {@link CesiumGeometry::Rectangle}.
   */
  constexpr CesiumGeometry::Rectangle toSimpleRectangle() const noexcept {
    return CesiumGeometry::Rectangle(
        this->getWest(),
        this->getSouth(),
        this->getEast(),
        this->getNorth());
  }

  /**
   * @brief Computes the width of this rectangle.
   *
   * The result will be in radians, in the range [0, Pi*2].
   */
  constexpr double computeWidth() const noexcept {
    double east = this->_east;
    const double west = this->_west;
    if (east < west) {
      east += CesiumUtility::Math::TwoPi;
    }
    return east - west;
  }

  /**
   * @brief Computes the height of this rectangle.
   *
   * The result will be in radians, in the range [0, Pi*2].
   */
  constexpr double computeHeight() const noexcept {
    return this->_north - this->_south;
  }

  /**
   * @brief Computes the {@link Cartographic} center position of this rectangle.
   */
  Cartographic computeCenter() const noexcept;

  /**
   * @brief Returns `true` if this rectangle contains the given point.
   *
   * The provided cartographic position must be within the longitude range [-Pi,
   * Pi] and the latitude range [-Pi/2, Pi/2].
   *
   * This will take into account the wrapping of the longitude at the
   * anti-meridian.
   */
  bool contains(const Cartographic& cartographic) const noexcept;

  /**
   * @brief Determines if this rectangle is empty.
   *
   * An empty rectangle bounds no part of the globe, not even a single point.
   */
  bool isEmpty() const noexcept;

  /**
   * @brief Computes the intersection of two rectangles.
   *
   * This function assumes that the rectangle's coordinates are latitude and
   * longitude in radians and produces a correct intersection, taking into
   * account the fact that the same angle can be represented with multiple
   * values as well as the wrapping of longitude at the anti-meridian.  For a
   * simple intersection that ignores these factors and can be used with
   * projected coordinates, see
   * {@link CesiumGeometry::Rectangle::computeIntersection}.
   *
   * @param other The other rectangle to intersect with this one.
   * @returns The intersection rectangle, or `std::nullopt` if there is no
   * intersection.
   */
  std::optional<GlobeRectangle>
  computeIntersection(const GlobeRectangle& other) const noexcept;

  /**
   * @brief Computes the union of this globe rectangle with another.
   *
   * @param other The other globe rectangle.
   * @return The union.
   */
  GlobeRectangle computeUnion(const GlobeRectangle& other) const noexcept;

  /**
   * @brief Splits this rectangle at the anti-meridian (180 degrees longitude),
   * if necessary.
   *
   * If the rectangle does not cross the anti-meridian, the entire rectangle is
   * returned in the `first` field of the pair and the `second` is std::nullopt.
   * If it does cross the anti-meridian, this function returns two rectangles
   * that touch but do not cross it. The larger of the two rectangles is
   * returned in `first` and the smaller one is returned in `second`.
   */
  std::pair<GlobeRectangle, std::optional<GlobeRectangle>>
  splitAtAntiMeridian() const noexcept;

  /**
   * @brief Checks whether two globe rectangles are exactly equal.
   *
   * @param left The first rectangle.
   * @param right The second rectangle.
   * @return Whether the rectangles are equal
   */
  static bool
  equals(const GlobeRectangle& left, const GlobeRectangle& right) noexcept;

  /**
   * @brief Checks whether two globe rectangles are equal up to a given relative
   * epsilon.
   *
   * @param left The first rectangle.
   * @param right The second rectangle.
   * @param relativeEpsilon The relative epsilon.
   * @return Whether the rectangles are epsilon-equal
   */
  static bool equalsEpsilon(
      const GlobeRectangle& left,
      const GlobeRectangle& right,
      double relativeEpsilon) noexcept;

private:
  double _west;
  double _south;
  double _east;
  double _north;
};

} // namespace CesiumGeospatial
