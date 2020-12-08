#pragma once

struct cgltf_image;

namespace CesiumGltf {
    class GltfImage {
    public:
        static GltfImage createFromCollectionElement(cgltf_image* array, size_t arrayIndex);

    private:
        GltfImage(cgltf_image* p);

        cgltf_image* _p;
    };
}
