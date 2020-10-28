#pragma once

#include <glm/gtc/epsilon.hpp>
#include <algorithm>
#include "CesiumUtility/Library.h"

namespace CesiumUtility {

    /**
     * @brief Mathematical constants and functions
     */
    class CESIUMUTILITY_API Math {
    public:

        /** @brief 0.1 */
        static const double EPSILON1;

        /** @brief 0.01 */
        static const double EPSILON2;

        /** @brief 0.001 */
        static const double EPSILON3;

        /** @brief 0.0001 */
        static const double EPSILON4;

        /** @brief 0.00001 */
        static const double EPSILON5;

        /** @brief 0.000001 */
        static const double EPSILON6;

        /** @brief 0.0000001 */
        static const double EPSILON7;

        /** @brief 0.00000001 */
        static const double EPSILON8;

        /** @brief 0.000000001 */
        static const double EPSILON9;

        /** @brief 0.0000000001 */
        static const double EPSILON10;

        /** @brief 0.00000000001 */
        static const double EPSILON11;

        /** @brief 0.000000000001 */
        static const double EPSILON12;

        /** @brief 0.0000000000001 */
        static const double EPSILON13;

        /** @brief 0.00000000000001 */
        static const double EPSILON14;

        /** @brief 0.000000000000001 */
        static const double EPSILON15;

        /** @brief 0.0000000000000001 */
        static const double EPSILON16;

        /** @brief 0.00000000000000001 */
        static const double EPSILON17;

        /** @brief 0.000000000000000001 */
        static const double EPSILON18;

        /** @brief 0.0000000000000000001 */
        static const double EPSILON19;

        /** @brief 0.00000000000000000001 */
        static const double EPSILON20;

        /** @brief 0.000000000000000000001 */
        static const double EPSILON21;

        /**
         * @brief pi
         */
        static const double ONE_PI;

        /**
         * @brief two times pi
         */
        static const double TWO_PI;

        /**
         * @brief pi divded by two
         */
        static const double PI_OVER_TWO;

        template<glm::length_t L, typename T, glm::qualifier Q>
        static inline glm::vec<L, T, Q> relativeEpsilonToAbsolute(const glm::vec<L, T, Q>& a, const glm::vec<L, T, Q>& b, double relativeEpsilon) {
            return relativeEpsilon * glm::max(glm::abs(a), glm::abs(b));
        }

        static inline double relativeEpsilonToAbsolute(double a, double b, double relativeEpsilon) {
            return relativeEpsilon * std::max(std::abs(a), std::abs(b));
        }

        template<glm::length_t L, typename T, glm::qualifier Q>
        static bool equalsEpsilon(const glm::vec<L, T, Q>& left, const glm::vec<L, T, Q>& right, double relativeEpsilon) {
            return Math::equalsEpsilon(left, right, relativeEpsilon, relativeEpsilon);
        }

        static inline bool equalsEpsilon(double left, double right, double relativeEpsilon) {
            return equalsEpsilon(left, right, relativeEpsilon, relativeEpsilon);
        }

        /**
         * Determines if two values are equal using an absolute or relative tolerance test. This is useful
         * to avoid problems due to roundoff error when comparing floating-point values directly. The values are
         * first compared using an absolute tolerance test. If that fails, a relative tolerance test is performed.
         * Use this test if you are unsure of the magnitudes of left and right.
         *
         * @param left The first value to compare.
         * @param right The other value to compare.
         * @param relativeEpsilon The maximum inclusive delta between `left` and `right` for the relative tolerance test.
         * @param absoluteEpsilon The maximum inclusive delta between `left` and `right` for the absolute tolerance test.
         * @returns `true` if the values are equal within the epsilon; otherwise, `false`.
         *
         * @snippet TestMath.cpp equalsEpsilon
         */
        static inline bool equalsEpsilon(double left, double right, double relativeEpsilon, double absoluteEpsilon) {
            double diff = std::abs(left - right);
            return
                diff <= absoluteEpsilon ||
                diff <= relativeEpsilonToAbsolute(left, right, relativeEpsilon);
        }

        template<glm::length_t L, typename T, glm::qualifier Q>
        static inline bool equalsEpsilon(const glm::vec<L, T, Q>& left, const glm::vec<L, T, Q>& right, double relativeEpsilon, double absoluteEpsilon) {
            glm::vec<L, T, Q> diff = glm::abs(left - right);
            return
                glm::lessThanEqual(diff, glm::vec<L, T, Q>(absoluteEpsilon)) == glm::vec<L, bool, Q>(true) ||
                glm::lessThanEqual(diff, relativeEpsilonToAbsolute(left, right, relativeEpsilon)) == glm::vec<L, bool, Q>(true);
        }

        /**
         * @brief Returns the sign of the value
         *
         * This is 1 if the value is positive, -1 if the value is
         * negative, or 0 if the value is 0.
         *
         * @param value The value to return the sign of.
         * @returns The sign of value.
         */
        static inline double sign(double value) {
            if (value == 0.0 || value != value) {
                // zero or NaN
                return value;
            }
            return value > 0 ? 1 : -1;
        }

        /**
         * @brief Returns 1.0 if the given value is positive or zero, and -1.0 if it is negative.
         *
         * This is similar to {@link Math::sign} except that returns 1.0 instead of
         * 0.0 when the input value is 0.0.
         *
         * @param value The value to return the sign of.
         * @returns The sign of value.
         */
        static inline double signNotZero(double value) {
            return value < 0.0 ? -1.0 : 1.0;
        }

        /**
         * @brief Produces an angle in the range -Pi <= angle <= Pi which is equivalent to the provided angle.
         *
         * @param angle The angle in radians
         * @returns The angle in the range [`-Math::ONE_PI`, `Math::ONE_PI`].
         */
        static inline double negativePiToPi(double angle) {
            return Math::zeroToTwoPi(angle + Math::ONE_PI) - Math::ONE_PI;
        }

        /**
         * @brief Produces an angle in the range 0 <= angle <= 2Pi which is equivalent to the provided angle.
         *
         * @param angle The angle in radians
         * @returns The angle in the range [0, `Math::TWO_PI`].
         */
        static inline double zeroToTwoPi(double angle) {
            double mod = Math::mod(angle, Math::TWO_PI);
            if (
                std::abs(mod) < Math::EPSILON14 &&
                std::abs(angle) > Math::EPSILON14
            ) {
                return Math::TWO_PI;
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
        static inline double mod(double m, double n) {
            return fmod(fmod(m, n) + n, n);
        }

        /**
         * @brief Converts degrees to radians.
         *
         * @param angleDegrees The angle to convert in degrees.
         * @returns The corresponding angle in radians.
         */
        static inline double degreesToRadians(double angleDegrees) {
            return angleDegrees * Math::ONE_PI / 180.0;
        }

        /**
         * @brief Converts radians to degrees.
         *
         * @param angleRadians The angle to convert in radians.
         * @returns The corresponding angle in degrees.
         */
        static inline double radiansToDegrees(double angleRadians) {
            return angleRadians * 180.0 / Math::ONE_PI;
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
        static inline double lerp(double p, double q, double time) {
            return (1.0 - time) * p + time * q;
        }

        /**
         * @brief Constrain a value to lie between two values.
         *
         * @param value The value to constrain.
         * @param min The minimum value.
         * @param max The maximum value.
         * @returns The value clamped so that min <= value <= max.
         */
        static inline double clamp(double value, double min, double max) {
            return value < min ? min : value > max ? max : value;
        };

        /**
         * @brief Converts a scalar value in the range [-1.0, 1.0] to a SNORM in the range [0, rangeMaximum]
         *
         * @param value The scalar value in the range [-1.0, 1.0]
         * @param rangeMaximum The maximum value in the mapped range, 255 by default.
         * @returns A SNORM value, where 0 maps to -1.0 and rangeMaximum maps to 1.0.
         *
         * @see Math::fromSNorm
         */
        static inline double toSNorm(double value, double rangeMaximum = 255.0) {
            return std::round(
                (Math::clamp(value, -1.0, 1.0) * 0.5 + 0.5) * rangeMaximum
            );
        };
        /**
         * @brief Converts a SNORM value in the range [0, rangeMaximum] to a scalar in the range [-1.0, 1.0].
         *
         * @param value SNORM value in the range [0, rangeMaximum]
         * @param rangeMaximum The maximum value in the SNORM range, 255 by default.
         * @returns Scalar in the range [-1.0, 1.0].
         *
         * @see Math::toSNorm
         */
        static inline double fromSNorm(double value, double rangeMaximum = 255.0) {
            return (Math::clamp(value, 0.0, rangeMaximum) / rangeMaximum) * 2.0 - 1.0;
        }


    };

}
