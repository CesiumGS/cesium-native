#pragma once

struct cgltf_buffer_view;

namespace CesiumGltf {
    class GltfBufferView {
    public:
        static GltfBufferView createFromCollectionElement(cgltf_buffer_view* array, size_t arrayIndex);

    private:
        GltfBufferView(cgltf_buffer_view* p);

        cgltf_buffer_view* _p;
    };
}
