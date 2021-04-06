#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/KHR_draco_mesh_compression.h"
#include "CesiumGltf/Reader.h"
#include "catch2/catch.hpp"
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <glm/vec3.hpp>
#include <gsl/span>
#include <rapidjson/reader.h>
#include <string>

using namespace CesiumGltf;

namespace {
std::vector<std::byte> readFile(const std::string& path) {
  FILE* fp = std::fopen(path.c_str(), "rb");
  REQUIRE(fp);

  try {
    std::fseek(fp, 0, SEEK_END);
    long pos = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);

    std::vector<std::byte> result(static_cast<size_t>(pos));
    size_t itemsRead = std::fread(result.data(), 1, result.size(), fp);
    REQUIRE(itemsRead == result.size());

    std::fclose(fp);

    return result;
  } catch (...) {
    if (fp) {
      std::fclose(fp);
    }
    throw;
  }
}
} // namespace

TEST_CASE("CesiumGltf::Reader") {
  using namespace std::string_literals;

  std::string s =
      "{"s + "  \"accessors\": ["s + "    {"s +
      "      \"count\": 4,"s + //{\"test\":true},"s +
      "      \"componentType\":5121,"s + "      \"type\":\"VEC2\","s +
      "      \"max\":[1.0, 2.2, 3.3],"s + "      \"min\":[0.0, -1.2]"s +
      "    }"s + "  ],"s + "  \"meshes\": [{"s + "    \"primitives\": [{"s +
      "      \"attributes\": {"s + "        \"POSITION\": 0,"s +
      "        \"NORMAL\": 1"s + "      },"s + "      \"targets\": ["s +
      "        {\"POSITION\": 10, \"NORMAL\": 11}"s + "      ]"s + "    }]"s +
      "  }],"s + "  \"surprise\":{\"foo\":true}"s + "}"s;
  ModelReaderResult result = CesiumGltf::readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()));
  CHECK(result.errors.empty());
  REQUIRE(result.model.has_value());

  Model& model = result.model.value();
  REQUIRE(model.accessors.size() == 1);
  CHECK(model.accessors[0].count == 4);
  CHECK(
      model.accessors[0].componentType ==
      Accessor::ComponentType::UNSIGNED_BYTE);
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

TEST_CASE("Read TriangleWithoutIndices") {
  std::filesystem::path gltfFile = CesiumGltfReader_TEST_DATA_DIR;
  gltfFile /= "TriangleWithoutIndices.gltf";
  std::vector<std::byte> data = readFile(gltfFile.string());
  ModelReaderResult result = CesiumGltf::readModel(data);
  REQUIRE(result.model);

  const Model& model = result.model.value();
  REQUIRE(model.meshes.size() == 1);
  REQUIRE(model.meshes[0].primitives.size() == 1);
  REQUIRE(model.meshes[0].primitives[0].attributes.size() == 1);
  REQUIRE(model.meshes[0].primitives[0].attributes.begin()->second == 0);

  AccessorView<glm::vec3> position(model, 0);
  REQUIRE(position.size() == 3);
  CHECK(position[0] == glm::vec3(0.0, 0.0, 0.0));
  CHECK(position[1] == glm::vec3(1.0, 0.0, 0.0));
  CHECK(position[2] == glm::vec3(0.0, 1.0, 0.0));
}

TEST_CASE("Nested extras serializes properly") {
  const std::string s = R"(
    {
        "asset" : {
            "version" : "1.1"
        },
        "extras": {
            "A": "Hello World",
            "B": 1234567,
            "C": {
                "C1": {},
                "C2": [1,2,3,4,5]
            }
        }
    }
  )";

  ModelReaderResult result = CesiumGltf::readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()));

  REQUIRE(result.errors.empty());
  REQUIRE(result.model.has_value());

  Model& model = result.model.value();
  auto cit = model.extras.find("C");
  REQUIRE(cit != model.extras.end());

  JsonValue* pC2 = cit->second.getValueForKey("C2");
  REQUIRE(pC2 != nullptr);

  CHECK(pC2->isArray());
  std::vector<JsonValue>& array = std::get<std::vector<JsonValue>>(pC2->value);
  CHECK(array.size() == 5);
  CHECK(array[0].getNumber(0.0) == 1.0);
  CHECK(array[1].getNumber(0.0) == 2.0);
  CHECK(array[2].getNumber(0.0) == 3.0);
  CHECK(array[3].getNumber(0.0) == 4.0);
  CHECK(array[4].getNumber(0.0) == 5.0);
}

TEST_CASE("Can deserialize KHR_draco_mesh_compression") {
  const std::string s = R"(
    {
      "asset": {
        "version": "2.0"
      },
      "meshes": [
        {
          "primitives": [
            {
              "extensions": {
                "KHR_draco_mesh_compression": {
                  "bufferView": 1,
                  "attributes": {
                    "POSITION": 0
                  }
                }
              }
            }
          ]
        }
      ]
    }
  )";

  ReadModelOptions options;
  ModelReaderResult modelResult = CesiumGltf::readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(modelResult.errors.empty());
  REQUIRE(modelResult.model.has_value());

  Model& model = modelResult.model.value();
  REQUIRE(model.meshes.size() == 1);
  REQUIRE(model.meshes[0].primitives.size() == 1);

  MeshPrimitive& primitive = model.meshes[0].primitives[0];
  KHR_draco_mesh_compression* pDraco =
      primitive.getExtension<KHR_draco_mesh_compression>();
  REQUIRE(pDraco);

  CHECK(pDraco->bufferView == 1);
  CHECK(pDraco->attributes.size() == 1);

  REQUIRE(pDraco->attributes.find("POSITION") != pDraco->attributes.end());
  CHECK(pDraco->attributes.find("POSITION")->second == 0);
}

TEST_CASE("Extensions deserialize to JsonVaue iff "
          "a default extension is registered") {
  const std::string s = R"(
    {
        "asset" : {
            "version" : "2.0"
        },
        "extensions": {
            "A": {
              "test": "Hello World"
            },
            "B": {
              "another": "Goodbye World"
            }
        }
    }
  )";

  ReadModelOptions options;
  ModelReaderResult withCustomExtModel = CesiumGltf::readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(withCustomExtModel.errors.empty());
  REQUIRE(withCustomExtModel.model.has_value());

  REQUIRE(withCustomExtModel.model->extensions.size() == 2);

  JsonValue* pA = withCustomExtModel.model->getGenericExtension("A");
  JsonValue* pB = withCustomExtModel.model->getGenericExtension("B");
  REQUIRE(pA != nullptr);
  REQUIRE(pB != nullptr);

  REQUIRE(pA->getValueForKey("test"));
  REQUIRE(pA->getValueForKey("test")->getString("") == "Hello World");

  REQUIRE(pB->getValueForKey("another"));
  REQUIRE(pB->getValueForKey("another")->getString("") == "Goodbye World");

  // Repeat test but this time the extension should be skipped.
  options.pExtensions =
      std::make_shared<ExtensionRegistry>(*options.pExtensions);
  options.pExtensions->clearDefault();

  ModelReaderResult withoutCustomExt = CesiumGltf::readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  auto& zeroExtensions = withoutCustomExt.model->extensions;
  REQUIRE(zeroExtensions.empty());
}
