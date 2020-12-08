#pragma once

struct cgltf_scene;

namespace CesiumGltf {
    class GltfScene {
    public:
        static GltfScene createFromCollectionElement(cgltf_scene* array, size_t arrayIndex);

    private:
        GltfScene(cgltf_scene* p);

        cgltf_scene* _p;
    };
}
