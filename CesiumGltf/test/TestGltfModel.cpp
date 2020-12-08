#include "catch2/catch.hpp"
#include "CesiumGltf/GltfModel.h"

using namespace CesiumGltf;

TEST_CASE("GltfModel") {
    std::vector<uint8_t> v;
    GltfModel model = GltfModel::fromMemory(v);
    GltfCollection<GltfMesh> meshes = model.meshes();
    for (const GltfMesh& mesh : meshes) {
        mesh;
    }

    for (const std::string& s : model.extensionsUsed()) {
        s;
    }
}