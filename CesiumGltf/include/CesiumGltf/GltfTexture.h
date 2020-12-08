#pragma once

struct cgltf_texture;

namespace CesiumGltf {
    class GltfTexture {
    public:
        static GltfTexture createFromCollectionElement(cgltf_texture* array, size_t arrayIndex);

    private:
        GltfTexture(cgltf_texture* p);

        cgltf_texture* _p;
    };
}
