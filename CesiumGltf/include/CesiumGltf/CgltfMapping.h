#pragma once

#include <string>

struct cgltf_accessor;
struct cgltf_animation;
struct cgltf_buffer;
struct cgltf_buffer_view;
struct cgltf_image;
struct cgltf_material;
struct cgltf_mesh;
struct cgltf_node;
struct cgltf_sampler;
struct cgltf_scene;
struct cgltf_texture;

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
        template <class T>
        struct CesiumToCgltf;

        template <> struct CesiumToCgltf<GltfAccessor> { using cgltf_type = cgltf_accessor; };
        template <> struct CesiumToCgltf<GltfAnimation> { using cgltf_type = cgltf_animation; };
        template <> struct CesiumToCgltf<GltfBuffer> { using cgltf_type = cgltf_buffer; };
        template <> struct CesiumToCgltf<GltfBufferView> { using cgltf_type = cgltf_buffer_view; };
        template <> struct CesiumToCgltf<GltfImage> { using cgltf_type = cgltf_image; };
        template <> struct CesiumToCgltf<GltfMaterial> { using cgltf_type = cgltf_material; };
        template <> struct CesiumToCgltf<GltfMesh> { using cgltf_type = cgltf_mesh; };
        template <> struct CesiumToCgltf<GltfNode> { using cgltf_type = cgltf_node; };
        template <> struct CesiumToCgltf<GltfSampler> { using cgltf_type = cgltf_sampler; };
        template <> struct CesiumToCgltf<GltfScene> { using cgltf_type = cgltf_scene; };
        template <> struct CesiumToCgltf<GltfTexture> { using cgltf_type = cgltf_texture; };
        template <> struct CesiumToCgltf<std::string> { using cgltf_type = char*; };

        template <class T>
        struct CesiumGltfObjectFactory {
            static T createFromCollectionElement(typename CesiumToCgltf<T>::cgltf_type* pElements, size_t currentElement) {
                return T::createFromCollectionElement(pElements, currentElement);
            }
        };

        template <>
        struct CesiumGltfObjectFactory<std::string> {
            static std::string createFromCollectionElement(char** pElements, size_t currentElement) {
                return std::string(pElements[currentElement]);
            }
        };
    }

}
