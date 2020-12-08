#pragma once

#include "CesiumGltf/GltfCollection.h"
#include "CesiumGltf/GltfAccessor.h"
#include "CesiumGltf/GltfAnimation.h"
#include "CesiumGltf/GltfBuffer.h"
#include "CesiumGltf/GltfBufferView.h"
#include "CesiumGltf/GltfImage.h"
#include "CesiumGltf/GltfMaterial.h"
#include "CesiumGltf/GltfMesh.h"
#include "CesiumGltf/GltfNode.h"
#include "CesiumGltf/GltfSampler.h"
#include "CesiumGltf/GltfScene.h"
#include "CesiumGltf/GltfTexture.h"
#include <gsl/span>
#include <memory>
#include <string>

struct cgltf_data;

namespace CesiumGltf {
    class GltfModel {
    public:
        static GltfModel fromMemory(gsl::span<const uint8_t> pData);

        GltfCollection<std::string> extensionsUsed() const noexcept;
        GltfCollection<std::string> extensionsRequired() const noexcept;
        GltfCollection<GltfAccessor> accessors() const noexcept;
        GltfCollection<GltfAnimation> animations() const noexcept;
        // GltfAsset asset() const noexcept;
        GltfCollection<GltfBuffer> buffers() const noexcept;
        GltfCollection<GltfBufferView> bufferViews() const noexcept;
        // GltfCollection<GltfCamera> cameras() const noexcept;
        GltfCollection<GltfImage> images() const noexcept;
        GltfCollection<GltfMaterial> materials() const noexcept;
        GltfCollection<GltfMesh> meshes() const noexcept;
        GltfCollection<GltfNode> nodes() const noexcept;
        GltfCollection<GltfSampler> samplers() const noexcept;
        // size_t scene() const noexcept;
        GltfCollection<GltfScene> scenes() const noexcept;
        // GltfCollection<GltfSkin> skins() const noexcept;
        GltfCollection<GltfTexture> textures() const noexcept;

    private:
        GltfModel(cgltf_data* pData);

        std::unique_ptr<cgltf_data, void (*)(cgltf_data*)> _pData;
    };

}
