#pragma once

struct cgltf_animation;

namespace CesiumGltf {
    class GltfAnimation {
    public:
        static GltfAnimation createFromCollectionElement(cgltf_animation* array, size_t arrayIndex);

    private:
        GltfAnimation(cgltf_animation* p);

        cgltf_animation* _p;
    };
}
