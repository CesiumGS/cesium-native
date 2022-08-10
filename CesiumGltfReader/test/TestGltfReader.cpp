#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/ExtensionKhrDracoMeshCompression.h>

#include <catch2/catch.hpp>
#include <glm/vec3.hpp>
#include <gsl/span>
#include <rapidjson/reader.h>

#include <filesystem>
#include <fstream>
#include <string>

using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace {
std::vector<std::byte> readFile(const std::filesystem::path& fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);
  REQUIRE(file);

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> buffer(static_cast<size_t>(size));
  file.read(reinterpret_cast<char*>(buffer.data()), size);

  return buffer;
}
} // namespace

TEST_CASE("CesiumGltfReader::GltfReader") {
  using namespace std::string_literals;

  std::string s = R"(
    {
      "accessors": [
        {
          "count": 4,
          "componentType": 5121,
          "type": "VEC2",
          "max": [
            1,
            2.2,
            3.3
          ],
          "min": [
            0,
            -1.2
          ]
        }
      ],
      "meshes": [
        {
          "primitives": [
            {
              "attributes": {
                "POSITION": 0,
                "NORMAL": 1
              },
              "targets": [
                {
                  "POSITION": 10,
                  "NORMAL": 11
                }
              ]
            }
          ]
        }
      ],
      "surprise": {
        "foo": true
      }
    }
  )";

  GltfReader reader;
  GltfReaderResult result = reader.readGltf(
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
  std::vector<std::byte> data = readFile(gltfFile);
  GltfReader reader;
  GltfReaderResult result = reader.readGltf(data);
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

TEST_CASE("Nested extras deserializes properly") {
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

  GltfReader reader;
  GltfReaderResult result = reader.readGltf(
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

  GltfReaderOptions options;
  GltfReader reader;
  GltfReaderResult result = reader.readGltf(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(result.errors.empty());
  REQUIRE(result.model.has_value());

  Model& model = result.model.value();
  REQUIRE(model.meshes.size() == 1);
  REQUIRE(model.meshes[0].primitives.size() == 1);

  MeshPrimitive& primitive = model.meshes[0].primitives[0];
  ExtensionKhrDracoMeshCompression* pDraco =
      primitive.getExtension<ExtensionKhrDracoMeshCompression>();
  REQUIRE(pDraco);

  CHECK(pDraco->bufferView == 1);
  CHECK(pDraco->attributes.size() == 1);

  REQUIRE(pDraco->attributes.find("POSITION") != pDraco->attributes.end());
  CHECK(pDraco->attributes.find("POSITION")->second == 0);

  // Repeat test but this time the extension should be deserialized as a
  // JsonValue.
  reader.getExtensions().setExtensionState(
      "KHR_draco_mesh_compression",
      CesiumJsonReader::ExtensionState::JsonOnly);

  GltfReaderResult result2 = reader.readGltf(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(result2.errors.empty());
  REQUIRE(result2.model.has_value());

  Model& model2 = result2.model.value();
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
  reader.getExtensions().setExtensionState(
      "KHR_draco_mesh_compression",
      CesiumJsonReader::ExtensionState::Disabled);

  GltfReaderResult result3 = reader.readGltf(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(result3.errors.empty());
  REQUIRE(result3.model.has_value());

  Model& model3 = result3.model.value();
  REQUIRE(model3.meshes.size() == 1);
  REQUIRE(model3.meshes[0].primitives.size() == 1);

  MeshPrimitive& primitive3 = model3.meshes[0].primitives[0];

  REQUIRE(!primitive3.getGenericExtension("KHR_draco_mesh_compression"));
  REQUIRE(!primitive3.getExtension<ExtensionKhrDracoMeshCompression>());
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

  GltfReaderOptions options;
  GltfReader reader;
  GltfReaderResult withCustomExtModel = reader.readGltf(
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
  reader.getExtensions().setExtensionState(
      "A",
      CesiumJsonReader::ExtensionState::Disabled);
  reader.getExtensions().setExtensionState(
      "B",
      CesiumJsonReader::ExtensionState::Disabled);

  GltfReaderResult withoutCustomExt = reader.readGltf(
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

  GltfReaderOptions options;
  GltfReader reader;
  GltfReaderResult result = reader.readGltf(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  // Note: The result.errors will not be empty,
  // because no images could be read.
  REQUIRE(result.model.has_value());
}

TEST_CASE("Can parse doubles with no fractions as integers") {
  std::string s = R"(
    {
      "accessors": [
        {
          "count": 4.0,
          "componentType": 5121.0
        }
      ]
    }
  )";

  GltfReaderOptions options;
  GltfReader reader;
  GltfReaderResult result = reader.readGltf(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  CHECK(result.warnings.empty());
  Model& model = result.model.value();
  CHECK(model.accessors[0].count == 4);
  CHECK(
      model.accessors[0].componentType ==
      Accessor::ComponentType::UNSIGNED_BYTE);
  s = R"(
    {
      "accessors": [
        {
          "count": 4.0,
          "componentType": 5121.1
        }
      ]
    }
  )";
  result = reader.readGltf(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);
  CHECK(!result.warnings.empty());
}

TEST_CASE("Test KTX2") {
  std::filesystem::path gltfFile = CesiumGltfReader_TEST_DATA_DIR;
  gltfFile /= "CesiumBalloonKTX2Hacky.glb";
  std::vector<std::byte> data = readFile(gltfFile.string());
  CesiumGltfReader::GltfReader reader;
  GltfReaderResult result = reader.readGltf(data);
  REQUIRE(result.model);

  const Model& model = result.model.value();
  REQUIRE(model.meshes.size() == 1);
}

TEST_CASE("Can apply RTC CENTER if model uses Cesium RTC extension") {
  const std::string s = R"(
    {
      "extensions": {
          "CESIUM_RTC": {
              "center": [6378137.0, 0.0, 0.0]
          }
      }
    }
  )";

  GltfReaderOptions options;
  GltfReader reader;
  GltfReaderResult result = reader.readGltf(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);
  REQUIRE(result.model.has_value());
  Model& model = result.model.value();
  const ExtensionCesiumRTC* cesiumRTC =
      model.getExtension<ExtensionCesiumRTC>();
  REQUIRE(cesiumRTC);
  std::vector<double> rtcCenter = {6378137.0, 0.0, 0.0};
  CHECK(cesiumRTC->center == rtcCenter);
}
