#pragma once

struct cgltf_node;

namespace CesiumGltf {
    class GltfNode {
    public:
        static GltfNode createFromCollectionElement(cgltf_node* array, size_t arrayIndex);

    private:
        GltfNode(cgltf_node* p);

        cgltf_node* _p;
    };
}
