#include "CesiumGltf/GltfReader.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "catch2/catch.hpp"

using namespace CesiumGltf;
using namespace CesiumUtility;

TEST_CASE("Can deserialize EXT_feature_metadata example with featureTables") {
  const std::string s = R"(
    {
      "asset": {
        "version": "2.0"
      },
      "extensions": {
        "EXT_feature_metadata": {
          "schema": {
            "classes": {
              "tree": {
                "properties": {
                  "height": {
                    "description": "Height of tree measured from ground level",
                    "type": "FLOAT32"
                  },
                  "birdCount": {
                    "description": "Number of birds perching on the tree",
                    "type": "UINT8"
                  },
                  "species": {
                    "description": "Species of the tree",
                    "type": "STRING"
                  }
                }
              }
            }
          },
          "featureTables": {
            "trees": {
              "class": "tree",
              "count": 10,
              "properties": {
                "height": {
                  "bufferView": 0
                },
                "birdCount": {
                  "bufferView": 1
                },
                "species": {
                  "bufferView": 2,
                  "stringOffsetBufferView": 3
                }
              }
            }
          }
        }
      }
    }
  )";

  ReadModelOptions options;
  CesiumGltf::GltfReader reader;
  ModelReaderResult modelResult = reader.readModel(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(modelResult.errors.empty());
  REQUIRE(modelResult.model.has_value());

  ModelEXT_feature_metadata* pMetadata =
      modelResult.model->getExtension<ModelEXT_feature_metadata>();
  REQUIRE(pMetadata);

  REQUIRE(pMetadata->schema.has_value());
  REQUIRE(pMetadata->schema->classes.size() == 1);

  auto treesIt = pMetadata->schema->classes.find("tree");
  REQUIRE(treesIt != pMetadata->schema->classes.end());

  REQUIRE(treesIt->second.properties.size() == 3);
}
