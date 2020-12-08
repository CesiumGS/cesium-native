#pragma once

struct cgltf_sampler;

namespace CesiumGltf {
    class GltfSampler {
    public:
        static GltfSampler createFromCollectionElement(cgltf_sampler* array, size_t arrayIndex);

    private:
        GltfSampler(cgltf_sampler* p);

        cgltf_sampler* _p;
    };
}
