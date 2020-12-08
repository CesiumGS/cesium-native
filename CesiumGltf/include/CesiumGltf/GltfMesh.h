#pragma once

struct cgltf_mesh;

namespace CesiumGltf {
    class GltfMesh {
    public:
        static GltfMesh createFromCollectionElement(cgltf_mesh* array, size_t arrayIndex);

    private:
        GltfMesh(cgltf_mesh* p);

        cgltf_mesh* _p;
    };
}
