#pragma once

namespace CesiumGltf {
    /**
     * @brief Specifies if the attribute is a scalar, vector, or matrix.
     */
    enum class AttributeType {
        /**
         * @brief The attribute is a scalar, i.e. a single numeric value.
         */
        SCALAR,

        /**
         * @brief The attribute is a 2D vector, i.e. two numeric values.
         */
        VEC2,

        /**
         * @brief The attribute is a 3D vector, i.e. three numeric values.
         */
        VEC3,

        /**
         * @brief The attribute is a 4D vector, i.e. four numeric values.
         */
        VEC4,

        /**
         * @brief The attribute is a 2x2 matrix in column-major order, i.e. four numeric values.
         */
        MAT2,

        /**
         * @brief The attribute is a 3x3 matrix in column-major order, i.e. nine numeric values.
         */
        MAT3,

        /**
         * @brief The attribute is a 4x4 matrix in column-major order, i.e. sixtreen numeric values.
         * 
         */
        MAT4
    };
}
