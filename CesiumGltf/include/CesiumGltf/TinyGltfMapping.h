#pragma once

#include <string>

// namespace tinygltf {
//     struct Accessor;
//     struct Animation;
//     struct Buffer;
//     struct BufferView;
//     struct Image;
//     struct Material;
//     struct Mesh;
//     struct Node;
//     struct Sampler;
//     struct Scene;
//     struct Texture;
// }

namespace CesiumGltf {

    class GltfAccessor;
    class GltfAnimation;
    class GltfBuffer;
    class GltfBufferView;
    class GltfImage;
    class GltfMaterial;
    class GltfMesh;
    class GltfNode;
    class GltfSampler;
    class GltfScene;
    class GltfTexture;

    namespace Impl {
        template <typename T>
        struct CesiumToTinyGltf {
            using tinygltf_type = typename T::tinygltf_type;
        };

        template <>
        struct CesiumToTinyGltf<std::string> {
            using tinygltf_type = std::string;
        };

        // template <> struct CesiumToTinyGltf<GltfAccessor> { using tinygltf_type = tinygltf::Accessor; };
        // template <> struct CesiumToTinyGltf<GltfAnimation> { using tinygltf_type = tinygltf::Animation; };
        // template <> struct CesiumToTinyGltf<GltfBuffer> { using tinygltf_type = tinygltf::Buffer; };
        // template <> struct CesiumToTinyGltf<GltfBufferView> { using tinygltf_type = tinygltf::BufferView; };
        // template <> struct CesiumToTinyGltf<GltfImage> { using tinygltf_type = tinygltf::Image; };
        // template <> struct CesiumToTinyGltf<GltfMaterial> { using tinygltf_type = tinygltf::Material; };
        // template <> struct CesiumToTinyGltf<GltfMesh> { using tinygltf_type = tinygltf::Mesh; };
        // template <> struct CesiumToTinyGltf<GltfNode> { using tinygltf_type = tinygltf::Node; };
        // template <> struct CesiumToTinyGltf<GltfSampler> { using tinygltf_type = tinygltf::Sampler; };
        // template <> struct CesiumToTinyGltf<GltfScene> { using tinygltf_type = tinygltf::Scene; };
        // template <> struct CesiumToTinyGltf<GltfTexture> { using tinygltf_type = tinygltf::Texture; };

        template <class T>
        struct CesiumGltfObjectFactory {
            static T createFromCollectionElement(typename T::tinygltf_type* pElement) {
                return T(pElement);
            }
        };

        template <>
        struct CesiumGltfObjectFactory<std::string> {
            static std::string createFromCollectionElement(std::string* pElement) {
                return *pElement;
            }
        };
    }

}
