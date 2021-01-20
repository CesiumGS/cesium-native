#include "catch2/catch.hpp"
#include "CesiumGltf/Reader.h"
#include <rapidjson/reader.h>
#include <gsl/span>
#include <string>

using namespace CesiumGltf;

TEST_CASE("CesiumGltf::Reader") {
    using namespace std::string_literals;

    std::string s =
        "{"s +
        "  \"accessors\": ["s +
        "    {"s +
        "      \"count\": 4,"s + //{\"test\":true},"s +
        "      \"componentType\":5121,"s +
        "      \"type\":\"VEC2\","s +
        "      \"max\":[1.0, 2.2, 3.3],"s +
        "      \"min\":[0.0, -1.2]"s +
        "    }"s +
        "  ],"s +
        "  \"meshes\": [{"s +
        "    \"primitives\": [{"s +
        "      \"attributes\": {"s +
        "        \"POSITION\": 0,"s +
        "        \"NORMAL\": 1"s +
        "      },"s +
        "      \"targets\": ["s +
        "        {\"POSITION\": 10, \"NORMAL\": 11}"s +
        "      ]"s +
        "    }]"s +
        "  }],"s +
        "  \"surprise\":{\"foo\":true}"s +
        "}"s;
    ModelReaderResult result = CesiumGltf::readModel(gsl::span(reinterpret_cast<const uint8_t*>(s.c_str()), s.size()));
    CHECK(result.errors.empty());
    REQUIRE(result.model.has_value());

    Model& model = result.model.value();
    REQUIRE(model.accessors.size() == 1);
    CHECK(model.accessors[0].count == 4);
    CHECK(model.accessors[0].componentType == Accessor::ComponentType::UNSIGNED_BYTE);
    CHECK(model.accessors[0].type == Accessor::Type::VEC2);
    REQUIRE(model.accessors[0].min.size() == 2);
    CHECK(model.accessors[0].min[0] == 0.0);
    CHECK(model.accessors[0].min[1] == -1.2);
    REQUIRE(model.accessors[0].max.size() == 3);
    CHECK(model.accessors[0].max[0] == 1.0);
    CHECK(model.accessors[0].max[1] == 2.2);
    CHECK(model.accessors[0].max[2] == 3.3);

    REQUIRE(model.meshes.size() == 1);
    REQUIRE(model.meshes[0].primitives.size() == 1);
    CHECK(model.meshes[0].primitives[0].attributes["POSITION"] == 0);
    CHECK(model.meshes[0].primitives[0].attributes["NORMAL"] == 1);

    REQUIRE(model.meshes[0].primitives[0].targets.size() == 1);
    CHECK(model.meshes[0].primitives[0].targets[0]["POSITION"] == 10);
    CHECK(model.meshes[0].primitives[0].targets[0]["NORMAL"] == 11);
}