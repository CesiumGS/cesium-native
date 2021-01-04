#pragma once

namespace CesiumGltf {
    /**
     * @brief The type of primitives to render.
     * 
     * All valid values correspond to WebGL enums.
     */
    enum class PrimitiveMode {
        /**
         * @brief Points.
         * 
         * A single dot at each vertex.
         */
        POINTS = 0,

        /**
         * @brief Lines.
         * 
         * Straight lines between pairs of vertices.
         */
        LINES = 1,

        /**
         * @brief A line loop.
         * 
         * Straight lines to each vertex in succession, and the last vertex connected back to the rfirst.
         */
        LINE_LOOP = 2,

        /**
         * @brief A line strip.
         * 
         * Straight lines to each vertex in succession.
         */
        LINE_STRIP = 3,

        /**
         * @brief Triangles.
         * 
         * A triangle for each group of three vertices.
         */
        TRIANGLES = 4,

        /**
         * @brief A triangle strip.
         * 
         * The first three vertices define the first triangle, and each single vertex thereafter defines an
         * additional triangle by connecting with the previous two vertices.
         */
        TRIANGLE_STRIP = 5,

        /**
         * @brief A triangle fan.
         * 
         * The first three vertices define the first triangle, and each successive vertex defines another triangle
         * fanning around the very first vertex.
         */
        TRIANGLE_FAN = 6
    };
}
