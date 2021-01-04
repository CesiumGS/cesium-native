#pragma once

#include "CesiumGltf/PrimitiveMode.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace CesiumGltf {
    struct Primitive {
        /**
         * @brief A dictionary object, where each key corresponds to mesh attribute semantic and each value is the index of the accessor containing attribute's data.
         */
        std::unordered_map<std::string, int32_t> attributes;

        /**
         * @brief The index of the accessor that contains the indices.
         * 
         * The index of the accessor in {@link Model::accessors} that contains mesh indices. When this is not defined (-1), the primitives should be
         * rendered without indices using `drawArrays()`. When defined, the accessor must contain indices: the {@link BufferView} referenced by the
         * accessor should have a {@link BufferView::target} equal to {@link BufferViewTarget::ELEMENT_ARRAY_BUFFER}; componentType must be
         * {@link ComponentType::UNSIGNED_BYTE}, {@link ComponentType::UNSIGNED_SHORT} or {@link ComponentType::UNSIGNED_INT}, the latter may
         * require enabling additional hardware support; {@link Accessor::type} must be {@link AccessorType::SCALAR}. For triangle primitives, the front face has a
         * counter-clockwise (CCW) winding order.
         * 
         * Values of the index accessor must not include the maximum value for the given component type, which triggers primitive restart in several graphics APIs
         * and would require client implementations to rebuild the index buffer. Primitive restart values are disallowed and all index values must refer to actual
         * vertices. As a result, the index accessor's values must not exceed the following maxima: {@link ComponentType::BYTE} `< 255`,
         * {@link ComponentType::UNSIGNED_SHORT} `< 65535`, {@link ComponentType::UNSIGNED_INT} `< 4294967295`.
         */
        int32_t indices = -1;

        /**
         * @brief The index of the material to apply to this primitive when rendering.
         * 
         * When this value is not defined (-1), the default material is used.
         */
        int32_t material = -1;

        /**
         * @brief The type of primitives to render.
         */
        PrimitiveMode mode = PrimitiveMode::TRIANGLES;

        /**
         * @brief An array of Morph Targets, each Morph Target is a dictionary mapping attributes
         * (only `POSITION`, `NORMAL`, and `TANGENT` supported) to their deviations in the Morph Target.
         */
        std::vector<std::unordered_map<std::string, size_t>> targets;
    };
}
