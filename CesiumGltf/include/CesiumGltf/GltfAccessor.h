#pragma once

namespace tinygltf {
    struct Accessor;
}

namespace CesiumGltf {
    template <typename T>
    class GltfCollection;

    class GltfAccessor final {
    public:
        GltfAccessor();
        ~GltfAccessor();

    private:
        using tinygltf_type = tinygltf::Accessor;

        GltfAccessor(tinygltf::Accessor* p);

        tinygltf::Accessor* _p;
        bool _owns;

        template <typename TElement>
        friend class GltfCollection;
    };
}
