#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumGltfReader/GltfSharedAssetSystem.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumUtility;
using namespace CesiumNativeTests;

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
      std::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()),
      options);

  REQUIRE(readerResult.errors.empty());
  REQUIRE(readerResult.model.has_value());

  ExtensionModelExtStructuralMetadata* pMetadata =
      readerResult.model->getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pMetadata);

  REQUIRE(pMetadata->schema != nullptr);
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

TEST_CASE("Can load an external structural metadata schema from a URI") {
  const std::string schema = R"(
  {
    "classes" : {
      "exampleMetadataClass" : {
        "name" : "Example metadata class",
        "description" : "An example metadata class for property attributes",
        "properties" : {
          "intensity" : {
            "name" : "Example intensity property",
            "description" : "An example property for the intensity, with component type FLOAT32",
            "type" : "SCALAR",
            "componentType" : "FLOAT32"
          },
          "classification" : {
            "name" : "Example classification property",
            "description" : "An example property for the classification, with the classificationEnumType",
            "type" : "ENUM",
            "enumType" : "classificationEnumType"
          }
        }
      }
    },
    "enums" : {
      "classificationEnumType" : {
        "valueType": "UINT16",
        "values" : [ {
          "name" : "MediumVegetation",
          "value" : 0
        }, {
          "name" : "Buildings",
          "value" : 1
        } ]
      }
    }
  }
  )";

  const std::string gltf = R"(
    {
      "extensions" : {
        "EXT_structural_metadata" : {
          "schemaUri" : "MetadataSchema.json",
          "propertyAttributes" : [ {
            "class" : "exampleMetadataClass",
            "properties" : {
              "intensity" : {
                "attribute" : "_INTENSITY"
              },
              "classification" : {
                "attribute" : "_CLASSIFICATION"
              }
            }
          } ]
        }
      },
      "extensionsUsed" : [ "EXT_structural_metadata" ],
      "asset" : {
        "version" : "2.0"
      }
    }
  )";

  auto pMockTaskProcessor = std::make_shared<SimpleTaskProcessor>();
  CesiumAsync::AsyncSystem asyncSystem{pMockTaskProcessor};

  auto pMockGltfBufferResponse = std::make_unique<SimpleAssetResponse>(
      uint16_t(200),
      "test.gltf",
      CesiumAsync::HttpHeaders{},
      std::vector<std::byte>(
          reinterpret_cast<const std::byte*>(gltf.c_str()),
          reinterpret_cast<const std::byte*>(gltf.c_str() + gltf.size())));
  auto pMockGltfBufferRequest = std::make_unique<SimpleAssetRequest>(
      "GET",
      "test.gltf",
      CesiumAsync::HttpHeaders{},
      std::move(pMockGltfBufferResponse));

  auto pMockSchemaResponse = std::make_unique<SimpleAssetResponse>(
      uint16_t(200),
      "MetadataSchema.json",
      CesiumAsync::HttpHeaders{},
      std::vector<std::byte>(
          reinterpret_cast<const std::byte*>(schema.c_str()),
          reinterpret_cast<const std::byte*>(schema.c_str() + schema.size())));
  auto pMockSchemaRequest = std::make_unique<SimpleAssetRequest>(
      "GET",
      "MetadataSchema.json",
      CesiumAsync::HttpHeaders{},
      std::move(pMockSchemaResponse));

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mapUrlToRequest{
      {"test.gltf", std::move(pMockGltfBufferRequest)},
      {"MetadataSchema.json", std::move(pMockSchemaRequest)}};
  auto pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mapUrlToRequest));

  CesiumGltfReader::GltfReaderOptions options;
  options.resolveExternalStructuralMetadata = true;
  options.pSharedAssetSystem =
      CesiumGltfReader::GltfSharedAssetSystem::getDefault();
  CesiumGltfReader::GltfReader reader;
  CesiumGltfReader::GltfReaderResult readerResult =
      reader.loadGltf(asyncSystem, "test.gltf", {}, pMockAssetAccessor, options)
          .waitInMainThread();

  REQUIRE(readerResult.errors.empty());
  REQUIRE(readerResult.model.has_value());

  ExtensionModelExtStructuralMetadata* pMetadata =
      readerResult.model->getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pMetadata);

  REQUIRE(pMetadata->schema != nullptr);
  REQUIRE(pMetadata->schema->classes.size() == 1);
  auto it = pMetadata->schema->classes.find("exampleMetadataClass");
  REQUIRE(it != pMetadata->schema->classes.end());
  REQUIRE(it->second.properties.size() == 2);

  REQUIRE(pMetadata->schema->enums.size() == 1);
  REQUIRE(
      pMetadata->schema->enums.find("classificationEnumType") !=
      pMetadata->schema->enums.end());
}
