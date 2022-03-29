#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/ExtensionModelExtFeatureMetadata.h>

#include <catch2/catch.hpp>

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
                    "type": "UINT8",
                    "min": 1
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

  CesiumGltfReader::GltfReaderOptions options;
  CesiumGltfReader::GltfReader reader;
  CesiumGltfReader::GltfReaderResult readerResult = reader.readGltf(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(readerResult.errors.empty());
  REQUIRE(readerResult.model.has_value());

  ExtensionModelExtFeatureMetadata* pMetadata =
      readerResult.model->getExtension<ExtensionModelExtFeatureMetadata>();
  REQUIRE(pMetadata);

  REQUIRE(pMetadata->schema.has_value());
  REQUIRE(pMetadata->schema->classes.size() == 1);

  auto treesIt = pMetadata->schema->classes.find("tree");
  REQUIRE(treesIt != pMetadata->schema->classes.end());

  REQUIRE(treesIt->second.properties.size() == 3);

  auto birdCountIt = treesIt->second.properties.find("birdCount");
  REQUIRE(birdCountIt != treesIt->second.properties.end());
  REQUIRE(!birdCountIt->second.max.has_value());
  REQUIRE(birdCountIt->second.min.has_value());
  REQUIRE(birdCountIt->second.min->getSafeNumberOrDefault(-1) == 1);
}
