#pragma once

#include "AttributeType.h"
#include "ComponentType.h"
#include <cstdint>
#include <vector>

namespace CesiumGltf {
    /**
     * @brief A typed view into a {@link BufferView}.
     * 
     * A {@link BufferView} contains raw binary data. An accessor provides a typed view into a
     * `BufferView` or a subset of a `BufferView` similar to how WebGL's `vertexAttribPointer()`
     * defines an attribute in a buffer.
     */
    struct Accessor {
        /**
         * @brief The index of the `BufferView` in {@link Model::bufferViews}.
         * 
         * When this value is less than 0, no `BufferView` is associated with this accessor.
         * The accessor should be treated as being initialized with all zeros. The `sparse`
         * property or extensions may override the zeros with actual values.
         */
        int32_t bufferView = -1;

        /**
         * @brief The offset relative to the start of the {@link Accessor::bufferView} in bytes.
         * 
         * This must be a multiple of the size of the component datatype.
         */
        int64_t byteOffset = 0;

        /**
         * @brief The datatype of components in the attribute.
         */
        ComponentType componentType = ComponentType::FLOAT;

        /**
         * @brief Specifies whether integer data values should be normalized.
         * 
         * Specifies whether integer data values should be normalized (`true`) to [0, 1] (for unsigned types) or
         * [-1, 1] (for signed types), or converted directly (`false`) when they are accessed. This property is
         * meaningful only for accessors that contain vertex attributes or animation output data.
         */
        bool normalized = false;

        /**
         * @brief The number of attributes referenced by this accessor.
         * 
         * Not to be confused with the number of bytes or number of components.
         */
        int64_t count = 0;

        /**
         * @brief Specifies if the attribute is a scalar, vector, or matrix.
         */
        AttributeType type = AttributeType::SCALAR;

        /**
         * @brief Maximum value of each component in this attribute.
         * 
         * Array elements must be treated as having the same data type as accessor's {@link Accessor::componentType}. Both min
         * and max arrays have the same length. The length is determined by the value of the type property; it can be 1, 2, 3, 4, 9, or 16.
         * 
         * The {@link Accessor::normalized} property has no effect on array values: they always correspond to the actual values stored in the buffer.
         * When accessor is sparse, this property must contain max values of accessor data with sparse substitution applied.
         */
        std::vector<double> max;

        /**
         * @brief Minimum value of each component in this attribute.
         * 
         * Array elements must be treated as having the same data type as accessor's {@link Accessor::componentType}. Both min
         * and max arrays have the same length. The length is determined by the value of the type property; it can be 1, 2, 3, 4, 9, or 16.
         * 
         * The {@link Accessor::normalized} property has no effect on array values: they always correspond to the actual values stored in the buffer.
         * When accessor is sparse, this property must contain min values of accessor data with sparse substitution applied.
         */
        std::vector<double> min;
    };
}
