#include "QuantizedMesh.h"
#include "tiny_gltf.h"
#include <stdexcept>

namespace Cesium3DTiles {
    std::string QuantizedMesh::CONTENT_TYPE = "application/vnd.quantized-mesh";

    std::unique_ptr<GltfContent> QuantizedMesh::load(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url) {
        tinygltf::Model model;
        return std::make_unique<GltfContent>(tile, model, url);
    }

}
