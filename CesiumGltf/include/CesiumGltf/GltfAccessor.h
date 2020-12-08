#pragma once

struct cgltf_accessor;

namespace CesiumGltf {
    class GltfAccessor {
    public:
        static GltfAccessor createFromCollectionElement(cgltf_accessor* array, size_t arrayIndex);

    private:
        GltfAccessor(cgltf_accessor* p);

        cgltf_accessor* _p;
    };
}
