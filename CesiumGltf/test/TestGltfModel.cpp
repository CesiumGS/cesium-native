#include "catch2/catch.hpp"
#include "CesiumGltf/Reader.h"
#include <rapidjson/reader.h>
#include <gsl/span>
#include <string>

using namespace CesiumGltf;

TEST_CASE("GltfModel") {
    std::string s = "{\"accessors\": [{\"count\": 4,\"componentType\":5121,\"type\":\"VEC2\",\"max\":[1.0, 2.2, 3.3],\"min\":[0.0, -1.2]}],\"surprise\":{\"foo\":true}}";
    ModelReaderResult result = CesiumGltf::readModel(gsl::span(reinterpret_cast<const uint8_t*>(s.c_str()), s.size()));
    Model& model = result.model;
    REQUIRE(model.accessors.size() == 1);
    CHECK(model.accessors[0].count == 4);
    CHECK(model.accessors[0].componentType == ComponentType::UNSIGNED_BYTE);
    CHECK(model.accessors[0].type == AttributeType::VEC2);
    REQUIRE(model.accessors[0].min.size() == 2);
    CHECK(model.accessors[0].min[0] == 0.0);
    CHECK(model.accessors[0].min[1] == -1.2);
    REQUIRE(model.accessors[0].max.size() == 3);
    CHECK(model.accessors[0].max[0] == 1.0);
    CHECK(model.accessors[0].max[1] == 2.2);
    CHECK(model.accessors[0].max[2] == 3.3);
    // std::vector<uint8_t> v;
    // GltfModel model = GltfModel::fromMemory(v);
    // GltfCollection<GltfMesh> meshes = model.meshes();
    // for (const GltfMesh& mesh : meshes) {
    //     mesh;
    // }

    // for (const std::string& s : model.extensionsUsed()) {
    //     s;
    // }
}