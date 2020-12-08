#pragma once

struct cgltf_material;

namespace CesiumGltf {
    class GltfMaterial {
    public:
        static GltfMaterial createFromCollectionElement(cgltf_material* array, size_t arrayIndex);

    private:
        GltfMaterial(cgltf_material* p);

        cgltf_material* _p;
    };
}
