#pragma once

namespace CesiumGltf {
    /**
     * @brief The datatype of components in the attribute.
     * 
     * The datatype of components in the attribute.  All valid values correspond to WebGL enums.
     * The corresponding typed arrays are `Int8Array`, `Uint8Array`, `Int16Array`, `Uint16Array`, `Uint32Array`,
     * and `Float32Array`, respectively.  5125 (UNSIGNED_INT) is only allowed when the accessor contains
     * indices, i.e., the accessor is only referenced by `primitive.indices`.
     */
    enum class ComponentType {
        /**
         * @brief A single-byte (8-bit), signed integer.
         * 
         * Corresponds to C++11's `int8_t` or JavaScript's `Int8Array`.
         */
        BYTE = 5120,

        /**
         * @brief A single-byte (8-bit), unsigned integer.
         * 
         * Corresponds to C++11's `uint8_t` or JavaScript's `Uint8Array`.
         */
        UNSIGNED_BYTE = 5121,

        /**
         * @brief A two-byte (16-bit), signed integer.
         * 
         * Corresponds to C++11's `int16_t` or JavaScript's `Int16Array`.
         */
        SHORT = 5122,

        /**
         * @brief A two-byte (16-bit), unsigned integer.
         * 
         * Corresponds to C++11's `uint16_t` or JavaScript's `Uint16Array`.
         */
        UNSIGNED_SHORT = 5123,

        /**
         * @brief A four-byte (32-bit), unsigned integer.
         * 
         * Corresponds to C++11's `uint32_t` or JavaScript's `Uint32Array`.
         * 
         * This component type is only allowed for accessors that only contain indices,
         * i.e. the accessor is only referenced by `primitive.indices`.
         */
        UNSIGNED_INT = 5125,

        /**
         * @brief A four-byte (32-bit), floating-point number.
         * 
         * Corresponds to C++'s `float` or JavaScript's `Float32Array`.
         */
        FLOAT = 5126
    };
}
