#include "CesiumGltf/GltfReader.h"

#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/KHR_draco_mesh_compression.h>

#include <catch2/catch.hpp>
#include <glm/vec3.hpp>
#include <gsl/span>
#include <rapidjson/reader.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

using namespace CesiumGltf;
using namespace CesiumUtility;

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

TEST_CASE("CesiumGltf::GltfReader") {
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
  CesiumGltf::GltfReader reader;
  ModelReaderResult result = reader.readModel(
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
  gltfFile /=
      "TriangleWithoutIndices/glTF-Embedded/TriangleWithoutIndices.gltf";
  std::vector<std::byte> data = readFile(gltfFile.string());
  CesiumGltf::GltfReader reader;
  ModelReaderResult result = reader.readModel(data);
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

TEST_CASE("Read BoxTexturedWebp (with error messages)") {
  std::filesystem::path gltfFile = CesiumGltfReader_TEST_DATA_DIR;
  gltfFile /= "BoxTexturedWebp/glTF/BoxTexturedWebp.gltf";
  std::vector<std::byte> data = readFile(gltfFile.string());
  CesiumGltf::GltfReader reader;
  ModelReaderResult result = reader.readModel(data);
  REQUIRE(result.model);
  REQUIRE(result.warnings.empty());

  // Expect errors, because WebP cannot be read
  REQUIRE(result.errors.size() > 0);
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

  CesiumGltf::GltfReader reader;
  ModelReaderResult result = reader.readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()));

  REQUIRE(result.errors.empty());
  REQUIRE(result.model.has_value());

  Model& model = result.model.value();
  auto cit = model.extras.find("C");
  REQUIRE(cit != model.extras.end());

  JsonValue* pC2 = cit->second.getValuePtrForKey("C2");
  REQUIRE(pC2 != nullptr);

  CHECK(pC2->isArray());
  std::vector<JsonValue>& array = std::get<std::vector<JsonValue>>(pC2->value);
  CHECK(array.size() == 5);
  CHECK(array[0].getSafeNumber<double>() == 1.0);
  CHECK(array[1].getSafeNumber<std::uint64_t>() == 2);
  CHECK(array[2].getSafeNumber<std::uint8_t>() == 3);
  CHECK(array[3].getSafeNumber<std::int16_t>() == 4);
  CHECK(array[4].getSafeNumber<std::int32_t>() == 5);
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
  CesiumGltf::GltfReader reader;
  ModelReaderResult modelResult = reader.readModel(
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

  // Repeat test but this time the extension should be deserialized as a
  // JsonValue.
  reader.setExtensionState(
      "KHR_draco_mesh_compression",
      ExtensionState::JsonOnly);

  ModelReaderResult modelResult2 = reader.readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(modelResult2.errors.empty());
  REQUIRE(modelResult2.model.has_value());

  Model& model2 = modelResult2.model.value();
  REQUIRE(model2.meshes.size() == 1);
  REQUIRE(model2.meshes[0].primitives.size() == 1);

  MeshPrimitive& primitive2 = model2.meshes[0].primitives[0];
  JsonValue* pDraco2 =
      primitive2.getGenericExtension("KHR_draco_mesh_compression");
  REQUIRE(pDraco2);

  REQUIRE(pDraco2->getValuePtrForKey("bufferView"));
  CHECK(
      pDraco2->getValuePtrForKey("bufferView")
          ->getSafeNumberOrDefault<int64_t>(0) == 1);

  REQUIRE(pDraco2->getValuePtrForKey("attributes"));
  REQUIRE(pDraco2->getValuePtrForKey("attributes")->isObject());
  REQUIRE(
      pDraco2->getValuePtrForKey("attributes")->getValuePtrForKey("POSITION"));
  REQUIRE(
      pDraco2->getValuePtrForKey("attributes")
          ->getValuePtrForKey("POSITION")
          ->getSafeNumberOrDefault<int64_t>(1) == 0);

  // Repeat test but this time the extension should not be deserialized at all.
  reader.setExtensionState(
      "KHR_draco_mesh_compression",
      ExtensionState::Disabled);

  ModelReaderResult modelResult3 = reader.readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(modelResult3.errors.empty());
  REQUIRE(modelResult3.model.has_value());

  Model& model3 = modelResult3.model.value();
  REQUIRE(model3.meshes.size() == 1);
  REQUIRE(model3.meshes[0].primitives.size() == 1);

  MeshPrimitive& primitive3 = model3.meshes[0].primitives[0];

  REQUIRE(!primitive3.getGenericExtension("KHR_draco_mesh_compression"));
  REQUIRE(!primitive3.getExtension<KHR_draco_mesh_compression>());
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
  CesiumGltf::GltfReader reader;
  ModelReaderResult withCustomExtModel = reader.readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(withCustomExtModel.errors.empty());
  REQUIRE(withCustomExtModel.model.has_value());

  REQUIRE(withCustomExtModel.model->extensions.size() == 2);

  JsonValue* pA = withCustomExtModel.model->getGenericExtension("A");
  JsonValue* pB = withCustomExtModel.model->getGenericExtension("B");
  REQUIRE(pA != nullptr);
  REQUIRE(pB != nullptr);

  REQUIRE(pA->getValuePtrForKey("test"));
  REQUIRE(
      pA->getValuePtrForKey("test")->getStringOrDefault("") == "Hello World");

  REQUIRE(pB->getValuePtrForKey("another"));
  REQUIRE(
      pB->getValuePtrForKey("another")->getStringOrDefault("") ==
      "Goodbye World");

  // Repeat test but this time the extension should be skipped.
  reader.setExtensionState("A", ExtensionState::Disabled);
  reader.setExtensionState("B", ExtensionState::Disabled);

  ModelReaderResult withoutCustomExt = reader.readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  auto& zeroExtensions = withoutCustomExt.model->extensions;
  REQUIRE(zeroExtensions.empty());
}

TEST_CASE("Unknown MIME types are handled") {
  const std::string s = R"(
    {
        "asset" : {
            "version" : "2.0"
        },
        "images": [
            {
              "mimeType" : "image/webp"
            }
        ]
    }
  )";

  ReadModelOptions options;
  CesiumGltf::GltfReader reader;
  ModelReaderResult modelResult = reader.readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  // Note: The modelResult.errors will not be empty,
  // because no images could be read.
  REQUIRE(modelResult.model.has_value());
}
