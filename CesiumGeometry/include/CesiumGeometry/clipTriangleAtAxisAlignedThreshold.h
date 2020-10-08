#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace CesiumGeometry {
    /**
     * A vertex resulting from clipping a triangle against a threshold. It may either be a
     * simple index referring to an existing vertex (identified with an index value of
     * 0, 1, or 2), or if the index is -1, it is an interpolation between two vertices.
     */
    struct TriangleClipVertex
    {
        TriangleClipVertex(int index);
        TriangleClipVertex(int first, int second, double t);

        /**
         * The index of the existing vertex.
         */
        int index;

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
     * Splits a 2D triangle at given axis-aligned threshold value and returns the resulting
     * polygon on a given side of the threshold.  The resulting polygon may have 0, 1, 2,
     * 3, or 4 vertices.
     *
     * @param threshold The threshold coordinate value at which to clip the triangle.
     * @param keepAbove true to keep the portion of the triangle above the threshold, or false
     *                  to keep the portion below.
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
        double u0,
        double u1,
        double u2,
        std::vector<TriangleClipVertex>& result
    );
}
