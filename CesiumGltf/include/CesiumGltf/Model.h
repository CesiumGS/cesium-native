#pragma once

#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/ExtensibleObject.h"
#include "CesiumGltf/Mesh.h"
#include "CesiumGltf/Material.h"
#include <vector>

namespace CesiumGltf {
    /**
     * @brief A glTF model.
     */
    struct Model : public ExtensibleObject {
        /**
         * @brief An array of accessors.
         * 
         * An accessor is a typed view into a {@link BufferView}.
         */
        std::vector<Accessor> accessors;

        /**
         * @brief An array of meshes.
         * 
         * A mesh is a set of {@link Primitive} instances to be rendered.
         */
        std::vector<Mesh> meshes;

        /**
         * @brief An array of materials.
         * 
         * A material defines the appearance of a primitive.
         */
        std::vector<Material> materials;
    };
}
