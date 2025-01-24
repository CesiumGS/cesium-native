#pragma once

#include <CesiumUtility/Library.h>

#include <glm/gtc/epsilon.hpp>

namespace CesiumUtility {

/**
 * @brief Mathematical constants and functions
 */
class CESIUMUTILITY_API Math final {
public:
  /** @brief 0.1 */
  static constexpr double Epsilon1 = 1e-1;

  /** @brief 0.01 */
  static constexpr double Epsilon2 = 1e-2;

  /** @brief 0.001 */
  static constexpr double Epsilon3 = 1e-3;

  /** @brief 0.0001 */
  static constexpr double Epsilon4 = 1e-4;

  /** @brief 0.00001 */
  static constexpr double Epsilon5 = 1e-5;

  /** @brief 0.000001 */
  static constexpr double Epsilon6 = 1e-6;

  /** @brief 0.0000001 */
  static constexpr double Epsilon7 = 1e-7;

  /** @brief 0.00000001 */
  static constexpr double Epsilon8 = 1e-8;

  /** @brief 0.000000001 */
  static constexpr double Epsilon9 = 1e-9;

  /** @brief 0.0000000001 */
  static constexpr double Epsilon10 = 1e-10;

  /** @brief 0.00000000001 */
  static constexpr double Epsilon11 = 1e-11;

  /** @brief 0.000000000001 */
  static constexpr double Epsilon12 = 1e-12;

  /** @brief 0.0000000000001 */
  static constexpr double Epsilon13 = 1e-13;

  /** @brief 0.00000000000001 */
  static constexpr double Epsilon14 = 1e-14;

  /** @brief 0.000000000000001 */
  static constexpr double Epsilon15 = 1e-15;

  /** @brief 0.0000000000000001 */
  static constexpr double Epsilon16 = 1e-16;

  /** @brief 0.00000000000000001 */
  static constexpr double Epsilon17 = 1e-17;

  /** @brief 0.000000000000000001 */
  static constexpr double Epsilon18 = 1e-18;

  /** @brief 0.0000000000000000001 */
  static constexpr double Epsilon19 = 1e-19;

  /** @brief 0.00000000000000000001 */
  static constexpr double Epsilon20 = 1e-20;

  /** @brief 0.000000000000000000001 */
  static constexpr double Epsilon21 = 1e-21;

  /**
   * @brief Pi
   */
  static constexpr double OnePi = 3.14159265358979323846;

  /**
   * @brief Two times pi
   */
  static constexpr double TwoPi = OnePi * 2.0;

  /**
   * @brief Pi divided by two
   */
  static constexpr double PiOverTwo = OnePi / 2.0;

  /**
   * @brief Pi divided by four
   */
  static constexpr double PiOverFour = OnePi / 4.0;

  /**
   * @brief Converts a relative to an absolute epsilon, for the epsilon-equality
   * check between two values.
   *
   * @tparam L The length type.
   * @tparam T value value type.
   * @tparam Q The GLM qualifier type.
   *
   * @param a The first value.
   * @param b The second value.
   * @param relativeEpsilon The relative epsilon.
   * @return The absolute epsilon.
   */
  template <glm::length_t L, typename T, glm::qualifier Q>
  static constexpr glm::vec<L, T, Q> relativeEpsilonToAbsolute(
      const glm::vec<L, T, Q>& a,
      const glm::vec<L, T, Q>& b,
      double relativeEpsilon) noexcept {
    return relativeEpsilon * glm::max(glm::abs(a), glm::abs(b));
  }

  /**
   * @brief Converts a relative to an absolute epsilon, for the epsilon-equality
   * check between two values.
   *
   * @param a The first value.
   * @param b The second value.
   * @param relativeEpsilon The relative epsilon.
   * @return The absolute epsilon.
   */
  static constexpr double relativeEpsilonToAbsolute(
      double a,
      double b,
      double relativeEpsilon) noexcept {
    return relativeEpsilon * glm::max(glm::abs(a), glm::abs(b));
  }

  /**
   * @brief Checks whether two values are equal up to a given relative epsilon.
   *
   * @tparam L The length type.
   * @tparam T value value type.
   * @tparam Q The GLM qualifier type.
   *
   * @param left The first value.
   * @param right The second value.
   * @param relativeEpsilon The relative epsilon.
   * @return Whether the values are epsilon-equal
   */
  template <glm::length_t L, typename T, glm::qualifier Q>
  static bool constexpr equalsEpsilon(
      const glm::vec<L, T, Q>& left,
      const glm::vec<L, T, Q>& right,
      double relativeEpsilon) noexcept {
    return Math::equalsEpsilon(left, right, relativeEpsilon, relativeEpsilon);
  }

  /**
   * @brief Checks whether two values are equal up to a given relative epsilon.
   *
   * @param left The first value.
   * @param right The second value.
   * @param relativeEpsilon The relative epsilon.
   * @return Whether the values are epsilon-equal
   */
  static constexpr bool
  equalsEpsilon(double left, double right, double relativeEpsilon) noexcept {
    return equalsEpsilon(left, right, relativeEpsilon, relativeEpsilon);
  }

  /**
   * @brief Determines if two values are equal using an absolute or relative
   * tolerance test.
   *
   * This is useful to avoid problems due to roundoff error when comparing
   * floating-point values directly. The values are first compared using an
   * absolute tolerance test. If that fails, a relative tolerance test is
   * performed. Use this test if you are unsure of the magnitudes of left and
   * right.
   *
   * @param left The first value to compare.
   * @param right The other value to compare.
   * @param relativeEpsilon The maximum inclusive delta between `left` and
   * `right` for the relative tolerance test.
   * @param absoluteEpsilon The maximum inclusive delta between `left` and
   * `right` for the absolute tolerance test.
   * @returns `true` if the values are equal within the epsilon; otherwise,
   * `false`.
   *
   * @snippet TestMath.cpp equalsEpsilon
   */
  static constexpr bool equalsEpsilon(
      double left,
      double right,
      double relativeEpsilon,
      double absoluteEpsilon) noexcept {
    const double diff = glm::abs(left - right);
    return diff <= absoluteEpsilon ||
           diff <= relativeEpsilonToAbsolute(left, right, relativeEpsilon);
  }

  /**
   * @brief Determines if two values are equal using an absolute or relative
   * tolerance test.
   *
   * This is useful to avoid problems due to roundoff error when comparing
   * floating-point values directly. The values are first compared using an
   * absolute tolerance test. If that fails, a relative tolerance test is
   * performed. Use this test if you are unsure of the magnitudes of left and
   * right.
   *
   * @tparam L The length type.
   * @tparam T value value type.
   * @tparam Q The GLM qualifier type.
   *
   * @param left The first value to compare.
   * @param right The other value to compare.
   * @param relativeEpsilon The maximum inclusive delta between `left` and
   * `right` for the relative tolerance test.
   * @param absoluteEpsilon The maximum inclusive delta between `left` and
   * `right` for the absolute tolerance test.
   * @returns `true` if the values are equal within the epsilon; otherwise,
   * `false`.
   */
  template <glm::length_t L, typename T, glm::qualifier Q>
  static constexpr bool equalsEpsilon(
      const glm::vec<L, T, Q>& left,
      const glm::vec<L, T, Q>& right,
      double relativeEpsilon,
      double absoluteEpsilon) noexcept {
    const glm::vec<L, T, Q> diff = glm::abs(left - right);
    return glm::lessThanEqual(diff, glm::vec<L, T, Q>(absoluteEpsilon)) ==
               glm::vec<L, bool, Q>(true) ||
           glm::lessThanEqual(
               diff,
               relativeEpsilonToAbsolute(left, right, relativeEpsilon)) ==
               glm::vec<L, bool, Q>(true);
  }

  /**
   * @brief Returns the sign of the value.
   *
   * This is 1 if the value is positive, -1 if the value is
   * negative, or 0 if the value is 0.
   *
   * @param value The value to return the sign of.
   * @returns The sign of value.
   */
  static constexpr double sign(double value) noexcept {
    if (value == 0.0 || value != value) {
      // zero or NaN
      return value;
    }
    return value > 0 ? 1 : -1;
  }

  /**
   * @brief Returns 1.0 if the given value is positive or zero, and -1.0 if it
   * is negative.
   *
   * This is similar to {@link Math::sign} except that returns 1.0 instead of
   * 0.0 when the input value is 0.0.
   *
   * @param value The value to return the sign of.
   * @returns The sign of value.
   */
  static constexpr double signNotZero(double value) noexcept {
    return value < 0.0 ? -1.0 : 1.0;
  }

  /**
   * @brief Produces an angle in the range -Pi <= angle <= Pi which is
   * equivalent to the provided angle.
   *
   * @param angle The angle in radians.
   * @returns The angle in the range [`-Math::OnePi`, `Math::OnePi`].
   */
  static double negativePiToPi(double angle) noexcept {
    if (angle >= -Math::OnePi && angle <= Math::OnePi) {
      // Early exit if the input is already inside the range. This avoids
      // unnecessary math which could introduce floating point error.
      return angle;
    }
    return Math::zeroToTwoPi(angle + Math::OnePi) - Math::OnePi;
  }

  /**
   * @brief Produces an angle in the range 0 <= angle <= 2Pi which is equivalent
   * to the provided angle.
   *
   * @param angle The angle in radians.
   * @returns The angle in the range [0, `Math::TwoPi`].
   */
  static double zeroToTwoPi(double angle) noexcept {
    if (angle >= 0 && angle <= Math::TwoPi) {
      // Early exit if the input is already inside the range. This avoids
      // unnecessary math which could introduce floating point error.
      return angle;
    }
    const double mod = Math::mod(angle, Math::TwoPi);
    if (glm::abs(mod) < Math::Epsilon14 && glm::abs(angle) > Math::Epsilon14) {
      return Math::TwoPi;
    }
    return mod;
  }

  /**
   * @brief The modulo operation that also works for negative dividends.
   *
   * @param m The dividend.
   * @param n The divisor.
   * @returns The remainder.
   */
  static double mod(double m, double n) noexcept {
    if (Math::sign(m) == Math::sign(n) && glm::abs(m) < glm::abs(n)) {
      // Early exit if the input does not need to be modded. This avoids
      // unnecessary math which could introduce floating point error.
      return m;
    }
    return fmod(fmod(m, n) + n, n);
  }

  /**
   * @brief Converts degrees to radians.
   *
   * @param angleDegrees The angle to convert in degrees.
   * @returns The corresponding angle in radians.
   */
  static constexpr double degreesToRadians(double angleDegrees) noexcept {
    return angleDegrees * Math::OnePi / 180.0;
  }

  /**
   * @brief Converts radians to degrees.
   *
   * @param angleRadians The angle to convert in radians.
   * @returns The corresponding angle in degrees.
   */
  static constexpr double radiansToDegrees(double angleRadians) noexcept {
    return angleRadians * 180.0 / Math::OnePi;
  }

  /**
   * @brief Computes the linear interpolation of two values.
   *
   * @param p The start value to interpolate.
   * @param q The end value to interpolate.
   * @param time The time of interpolation generally in the range `[0.0, 1.0]`.
   * @returns The linearly interpolated value.
   *
   * @snippet TestMath.cpp lerp
   */
  static double lerp(double p, double q, double time) noexcept {
    return glm::mix(p, q, time);
  }

  /**
   * @brief Constrain a value to lie between two values.
   *
   * @param value The value to constrain.
   * @param min The minimum value.
   * @param max The maximum value.
   * @returns The value clamped so that min <= value <= max.
   */
  static constexpr double clamp(double value, double min, double max) noexcept {
    return glm::clamp(value, min, max);
  };

  /**
   * @brief Converts a scalar value in the range [-1.0, 1.0] to a SNORM in the
   * range [0, rangeMaximum]
   *
   * @param value The scalar value in the range [-1.0, 1.0].
   * @param rangeMaximum The maximum value in the mapped range, 255 by default.
   * @returns A SNORM value, where 0 maps to -1.0 and rangeMaximum maps to 1.0.
   *
   * @see Math::fromSNorm
   */
  static double toSNorm(double value, double rangeMaximum = 255.0) noexcept {
    return glm::round(
        (Math::clamp(value, -1.0, 1.0) * 0.5 + 0.5) * rangeMaximum);
  };
  /**
   * @brief Converts a SNORM value in the range [0, rangeMaximum] to a scalar in
   * the range [-1.0, 1.0].
   *
   * @param value SNORM value in the range [0, rangeMaximum].
   * @param rangeMaximum The maximum value in the SNORM range, 255 by default.
   * @returns Scalar in the range [-1.0, 1.0].
   *
   * @see Math::toSNorm
   */
  static constexpr double
  fromSNorm(double value, double rangeMaximum = 255.0) noexcept {
    return (Math::clamp(value, 0.0, rangeMaximum) / rangeMaximum) * 2.0 - 1.0;
  }

  /**
   * Converts a longitude value, in radians, to the range [`-Math::OnePi`,
   * `Math::OnePi`).
   *
   * @param angle The longitude value, in radians, to convert to the range
   * [`-Math::OnePi`, `Math::OnePi`).
   * @returns The equivalent longitude value in the range [`-Math::OnePi`,
   * `Math::OnePi`).
   *
   * @snippet TestMath.cpp convertLongitudeRange
   */
  static double convertLongitudeRange(double angle) noexcept {
    constexpr double twoPi = Math::TwoPi;

    const double simplified = angle - glm::floor(angle / twoPi) * twoPi;

    if (simplified < -Math::OnePi) {
      return simplified + twoPi;
    }
    if (simplified >= Math::OnePi) {
      return simplified - twoPi;
    }

    return simplified;
  };

  /**
   * @brief Rounds a value up to the nearest integer, like `ceil`, except
   * that if the value is very close to the lower integer it is rounded down
   * (like `floor`) instead.
   *
   * @param value The value to round.
   * @param tolerance The tolerance. If the value is closer than this to the
   * lower integer, it is rounded down instead.
   * @return The rounded value.
   */
  static double roundUp(double value, double tolerance) noexcept {
    const double up = glm::ceil(value);
    const double down = glm::floor(value);
    if (value - down < tolerance) {
      return down;
    } else {
      return up;
    }
  }

  /**
   * @brief Rounds a value down to the nearest integer, like `floor`, except
   * that if the value is very close to the higher integer it is rounded up
   * (like `ceil`) instead.
   *
   * @param value The value to round.
   * @param tolerance The tolerance. If the value is closer than this to the
   * higher integer, it is rounded up instead.
   * @return The rounded value.
   */
  static double roundDown(double value, double tolerance) noexcept {
    const double up = glm::ceil(value);
    const double down = glm::floor(value);
    if (up - value < tolerance) {
      return up;
    } else {
      return down;
    }
  }
};

} // namespace CesiumUtility
