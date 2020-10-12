#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <variant>

namespace CesiumGeometry {
    struct InterpolatedVertex {
        /**
         * The index of the first vertex to interpolate between. Ignored
         * if @link index is not -1.
         */
        int first;

        /**
         * The index of the second vertex to interpolate between. Ignored
         * if @link index is not -1.
         */
        int second;

        /**
         * The fraction of the distance from @link first to @link second at
         * which to interpolate. Ignored if @link index is not -1.
         */
        double t;
    };

    /**
     * A vertex resulting from clipping a triangle against a threshold. It may either be a
     * simple index referring to an existing vertex, or an interpolation between two vertices.
     */
    typedef std::variant<int, InterpolatedVertex> TriangleClipVertex;

    /**
     * Splits a 2D triangle at given axis-aligned threshold value and returns the resulting
     * polygon on a given side of the threshold.  The resulting polygon may have 0, 1, 2,
     * 3, or 4 vertices.
     *
     * @param threshold The threshold coordinate value at which to clip the triangle.
     * @param keepAbove true to keep the portion of the triangle above the threshold, or false
     *                  to keep the portion below.
     * @param i0 The index of the first vertex in the triangle in counter-clockwise order, used only to construct the TriangleClipVertex result.
     * @param i1 The index of the second vertex in the triangle in counter-clockwise order, used only to construct the TriangleClipVertex result.
     * @param i2 The index of the third vertex in the triangle in counter-clockwise order, used only to construct the TriangleClipVertex result.
     * @param u0 The coordinate of the first vertex in the triangle, in counter-clockwise order.
     * @param u1 The coordinate of the second vertex in the triangle, in counter-clockwise order.
     * @param u2 The coordinate of the third vertex in the triangle, in counter-clockwise order.
     * @param result On return, contains the polygon that results after the clip, specified as a list of
     *               vertices. If this vector already contains elements, the result is pushed onto the end
     *               of the vector.
     *
     * @example
     * TODO port this CesiumJS example to cesium-native
     * var result = Cesium.Intersections2D.clipTriangleAtAxisAlignedThreshold(0.5, false, 0.2, 0.6, 0.4);
     * // result === [2, 0, -1, 1, 0, 0.25, -1, 1, 2, 0.5]
     */
    void clipTriangleAtAxisAlignedThreshold(
        double threshold,
        bool keepAbove,
        int i0,
        int i1,
        int i2,
        double u0,
        double u1,
        double u2,
        std::vector<TriangleClipVertex>& result
    );
}
