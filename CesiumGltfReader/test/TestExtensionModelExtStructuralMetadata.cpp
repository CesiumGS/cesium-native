#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>

#include <catch2/catch.hpp>

using namespace CesiumGltf;
using namespace CesiumUtility;

TEST_CASE(
    "Can deserialize EXT_structural_metadata example with propertyTables") {
  const std::string s = R"(
    {
      "asset": {
        "version": "2.0"
      },
      "extensions": {
        "EXT_structural_metadata": {
          "schema": {
            "classes": {
              "tree": {
                "properties": {
                  "height": {
                    "description": "Height of tree measured from ground level",
                    "type": "SCALAR",
                    "componentType": "FLOAT32"
                  },
                  "birdCount": {
                    "description": "Number of birds perching on the tree",
                    "type": "SCALAR",
                    "componentType": "UINT8",
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
          "propertyTables": [
            {
              "class": "tree",
              "count": 10,
              "properties": {
                "height": {
                  "values": 0
                },
                "birdCount": {
                  "values": 1
                },
                "species": {
                  "values": 2,
                  "stringOffsets": 3
                }
              }
            }
          ]
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

  ExtensionModelExtStructuralMetadata* pMetadata =
      readerResult.model->getExtension<ExtensionModelExtStructuralMetadata>();
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
