#include <Cesium3DTiles/Buffer.h>
#include <Cesium3DTilesReader/SubtreeFileReader.h>
#include <Cesium3DTilesReader/SubtreeReader.h>
#include <Cesium3DTilesReader/SubtreesReader.h>
#include <Cesium3DTilesWriter/SubtreeWriter.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <doctest/doctest.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

using namespace CesiumNativeTests;

namespace {
void check(const std::string& input, const std::string& expectedOutput) {
  Cesium3DTilesReader::SubtreeReader reader;
  auto readResult = reader.readFromJson(std::span(
      reinterpret_cast<const std::byte*>(input.c_str()),
      input.size()));
  REQUIRE(readResult.errors.empty());
  REQUIRE(readResult.warnings.empty());
  REQUIRE(readResult.value.has_value());

  Cesium3DTiles::Subtree& subtree = readResult.value.value();

  Cesium3DTilesWriter::SubtreeWriter writer;
  Cesium3DTilesWriter::SubtreeWriterResult writeResult =
      writer.writeSubtreeJson(subtree);
  const auto subtreeBytes = writeResult.subtreeBytes;

  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  const std::string subtreeString(
      reinterpret_cast<const char*>(subtreeBytes.data()),
      subtreeBytes.size());

  rapidjson::Document subtreeJson;
  subtreeJson.Parse(subtreeString.c_str());

  rapidjson::Document expectedJson;
  expectedJson.Parse(expectedOutput.c_str());

  REQUIRE(subtreeJson == expectedJson);
}

bool hasSpaces(const std::string& input) {
  return std::count_if(input.begin(), input.end(), [](unsigned char c) {
    return std::isspace(c);
  });
}

struct ExtensionSubtreeTest final : public CesiumUtility::ExtensibleObject {
  static inline constexpr const char* ExtensionName = "PRIVATE_subtree_test";
};

} // namespace

TEST_CASE("Writes subtree JSON") {
  std::string string = R"(
    {
      "buffers": [
        {
          "name": "Availability Buffer",
          "uri": "availability.bin",
          "byteLength": 48
        },
        {
          "name": "Metadata Buffer",
          "uri": "metadata.bin",
          "byteLength": 6512
        }
      ],
      "bufferViews": [
        { "buffer": 0, "byteOffset": 0, "byteLength": 11 },
        { "buffer": 0, "byteOffset": 16, "byteLength": 32 },
        { "buffer": 1, "byteOffset": 0, "byteLength": 2040 },
        { "buffer": 1, "byteOffset": 2040, "byteLength": 1530 },
        { "buffer": 1, "byteOffset": 3576, "byteLength": 344 },
        { "buffer": 1, "byteOffset": 3920, "byteLength": 1024 },
        { "buffer": 1, "byteOffset": 4944, "byteLength": 240 },
        { "buffer": 1, "byteOffset": 5184, "byteLength": 122 },
        { "buffer": 1, "byteOffset": 5312, "byteLength": 480 },
        { "buffer": 1, "byteOffset": 5792, "byteLength": 480 },
        { "buffer": 1, "byteOffset": 6272, "byteLength": 240 }
      ],
      "propertyTables": [
        {
          "class": "tile",
          "count": 85,
          "properties": {
            "horizonOcclusionPoint": {
              "values": 2
            },
            "countries": {
              "values": 3,
              "arrayOffsets": 4,
              "stringOffsets": 5
            }
          }
        },
        {
          "class": "content",
          "count": 60,
          "properties": {
            "attributionIds": {
              "values": 6,
              "arrayOffsets": 7,
              "arrayOffsetType": "UINT16"
            },
            "minimumHeight": {
              "values": 8
            },
            "maximumHeight": {
              "values": 9
            },
            "triangleCount": {
              "values": 10,
              "min": 520,
              "max": 31902
            }
          }
        }
      ],
      "tileAvailability": {
        "constant": 1
      },
      "contentAvailability": [{
        "bitstream": 0,
        "availableCount": 60
      }],
      "childSubtreeAvailability": {
        "bitstream": 1
      },
      "tileMetadata": 0,
      "contentMetadata": [1],
      "subtreeMetadata": {
        "class": "subtree",
        "properties": {
          "attributionStrings": [
            "Source A",
            "Source B",
            "Source C",
            "Source D"
          ]
        }
      }
    }
  )";

  check(string, string);
}

TEST_CASE("Writes subtree JSON with extras") {
  std::string string = R"(
    {
      "tileAvailability": {
        "constant": 1
      },
      "contentAvailability": [{
        "constant": 1
      }],
      "childSubtreeAvailability": {
        "constant": 1
      },
      "extras": {
        "A": "Hello",
        "B": 1234567,
        "C": {
          "C1": {},
          "C2": [1,2,3,4,5],
          "C3": true
        }
      }
    }
  )";

  check(string, string);
}

TEST_CASE("Writes subtree JSON with custom extension") {
  std::string string = R"(
    {
      "tileAvailability": {
        "constant": 1
      },
      "contentAvailability": [{
        "constant": 1
      }],
      "childSubtreeAvailability": {
        "constant": 1
      },
      "extensions": {
        "A": {
          "test": "Hello"
        },
        "B": {
          "another": "Goodbye"
        }
      }
    }
  )";

  check(string, string);
}

TEST_CASE("Writes subtree JSON with unregistered extension") {
  Cesium3DTiles::Subtree subtree;
  subtree.addExtension<ExtensionSubtreeTest>();

  SUBCASE("Reports a warning if the extension is enabled") {
    Cesium3DTilesWriter::SubtreeWriter writer;
    Cesium3DTilesWriter::SubtreeWriterResult result =
        writer.writeSubtreeJson(subtree);
    REQUIRE(!result.warnings.empty());
  }

  SUBCASE("Does not report a warning if the extension is disabled") {
    Cesium3DTilesWriter::SubtreeWriter writer;
    writer.getExtensions().setExtensionState(
        ExtensionSubtreeTest::ExtensionName,
        CesiumJsonWriter::ExtensionState::Disabled);
    Cesium3DTilesWriter::SubtreeWriterResult result =
        writer.writeSubtreeJson(subtree);
    REQUIRE(result.warnings.empty());
  }
}

TEST_CASE("Writes subtree JSON with default values removed") {
  std::string string = R"(
    {
      "buffers": [
        {
          "name": "Availability Buffer",
          "uri": "availability.bin",
          "byteLength": 48
        },
        {
          "name": "Metadata Buffer",
          "uri": "metadata.bin",
          "byteLength": 6512
        }
      ],
      "bufferViews": [
        { "buffer": 0, "byteOffset": 0, "byteLength": 11 },
        { "buffer": 0, "byteOffset": 16, "byteLength": 32 },
        { "buffer": 1, "byteOffset": 0, "byteLength": 2040 },
        { "buffer": 1, "byteOffset": 2040, "byteLength": 1530 },
        { "buffer": 1, "byteOffset": 3576, "byteLength": 344 },
        { "buffer": 1, "byteOffset": 3920, "byteLength": 1024 },
        { "buffer": 1, "byteOffset": 4944, "byteLength": 240 },
        { "buffer": 1, "byteOffset": 5184, "byteLength": 122 },
        { "buffer": 1, "byteOffset": 5312, "byteLength": 480 },
        { "buffer": 1, "byteOffset": 5792, "byteLength": 480 },
        { "buffer": 1, "byteOffset": 6272, "byteLength": 240 }
      ],
      "propertyTables": [
        {
          "class": "tile",
          "count": 85,
          "properties": {
            "horizonOcclusionPoint": {
              "values": 2
            },
            "countries": {
              "values": 3,
              "arrayOffsets": 4,
              "stringOffsets": 5,
              "arrayOffsetType": "UINT32",
              "stringOffsetType": "UINT32"
            }
          }
        },
        {
          "class": "content",
          "count": 60,
          "properties": {
            "attributionIds": {
              "values": 6,
              "arrayOffsets": 7,
              "arrayOffsetType": "UINT16"
            },
            "minimumHeight": {
              "values": 8
            },
            "maximumHeight": {
              "values": 9
            },
            "triangleCount": {
              "values": 10,
              "min": 520,
              "max": 31902
            }
          }
        }
      ],
      "tileAvailability": {
        "constant": 1
      },
      "contentAvailability": [{
        "bitstream": 0,
        "availableCount": 60
      }],
      "childSubtreeAvailability": {
        "bitstream": 1
      },
      "tileMetadata": 0,
      "contentMetadata": [1],
      "subtreeMetadata": {
        "class": "subtree",
        "properties": {
          "attributionStrings": [
            "Source A",
            "Source B",
            "Source C",
            "Source D"
          ]
        }
      }
    }
  )";

  std::string expected = R"(
    {
      "buffers": [
        {
          "name": "Availability Buffer",
          "uri": "availability.bin",
          "byteLength": 48
        },
        {
          "name": "Metadata Buffer",
          "uri": "metadata.bin",
          "byteLength": 6512
        }
      ],
      "bufferViews": [
        { "buffer": 0, "byteOffset": 0, "byteLength": 11 },
        { "buffer": 0, "byteOffset": 16, "byteLength": 32 },
        { "buffer": 1, "byteOffset": 0, "byteLength": 2040 },
        { "buffer": 1, "byteOffset": 2040, "byteLength": 1530 },
        { "buffer": 1, "byteOffset": 3576, "byteLength": 344 },
        { "buffer": 1, "byteOffset": 3920, "byteLength": 1024 },
        { "buffer": 1, "byteOffset": 4944, "byteLength": 240 },
        { "buffer": 1, "byteOffset": 5184, "byteLength": 122 },
        { "buffer": 1, "byteOffset": 5312, "byteLength": 480 },
        { "buffer": 1, "byteOffset": 5792, "byteLength": 480 },
        { "buffer": 1, "byteOffset": 6272, "byteLength": 240 }
      ],
      "propertyTables": [
        {
          "class": "tile",
          "count": 85,
          "properties": {
            "horizonOcclusionPoint": {
              "values": 2
            },
            "countries": {
              "values": 3,
              "arrayOffsets": 4,
              "stringOffsets": 5
            }
          }
        },
        {
          "class": "content",
          "count": 60,
          "properties": {
            "attributionIds": {
              "values": 6,
              "arrayOffsets": 7,
              "arrayOffsetType": "UINT16"
            },
            "minimumHeight": {
              "values": 8
            },
            "maximumHeight": {
              "values": 9
            },
            "triangleCount": {
              "values": 10,
              "min": 520,
              "max": 31902
            }
          }
        }
      ],
      "tileAvailability": {
        "constant": 1
      },
      "contentAvailability": [{
        "bitstream": 0,
        "availableCount": 60
      }],
      "childSubtreeAvailability": {
        "bitstream": 1
      },
      "tileMetadata": 0,
      "contentMetadata": [1],
      "subtreeMetadata": {
        "class": "subtree",
        "properties": {
          "attributionStrings": [
            "Source A",
            "Source B",
            "Source C",
            "Source D"
          ]
        }
      }
    }
  )";

  check(string, expected);
}

TEST_CASE("Writes subtree JSON with prettyPrint") {
  Cesium3DTiles::Subtree subtree;

  Cesium3DTilesWriter::SubtreeWriter writer;
  Cesium3DTilesWriter::SubtreeWriterOptions options;
  options.prettyPrint = false;

  Cesium3DTilesWriter::SubtreeWriterResult writeResult =
      writer.writeSubtreeJson(subtree, options);
  const std::vector<std::byte>& subtreeBytesCompact = writeResult.subtreeBytes;

  std::string subtreeStringCompact(
      reinterpret_cast<const char*>(subtreeBytesCompact.data()),
      subtreeBytesCompact.size());

  REQUIRE_FALSE(hasSpaces(subtreeStringCompact));

  options.prettyPrint = true;
  writeResult = writer.writeSubtreeJson(subtree, options);
  const std::vector<std::byte>& subtreeBytesPretty = writeResult.subtreeBytes;
  std::string subtreeStringPretty(
      reinterpret_cast<const char*>(subtreeBytesPretty.data()),
      subtreeBytesPretty.size());

  REQUIRE(hasSpaces(subtreeStringPretty));
}

TEST_CASE("Writes subtree binary") {
  const std::vector<std::byte> bufferData{
      std::byte('H'),
      std::byte('e'),
      std::byte('l'),
      std::byte('l'),
      std::byte('o'),
      std::byte('W'),
      std::byte('o'),
      std::byte('r'),
      std::byte('l'),
      std::byte('d'),
      std::byte('!')};

  Cesium3DTiles::Subtree subtree;
  Cesium3DTiles::Buffer buffer;
  buffer.byteLength = static_cast<int64_t>(bufferData.size());
  subtree.buffers.push_back(buffer);

  Cesium3DTilesWriter::SubtreeWriter writer;
  Cesium3DTilesWriter::SubtreeWriterResult writeResult =
      writer.writeSubtreeBinary(subtree, std::span(bufferData));
  const std::vector<std::byte>& subtreeBytes = writeResult.subtreeBytes;

  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  // Now read the subtree back
  auto pMockTaskProcessor = std::make_shared<SimpleTaskProcessor>();
  CesiumAsync::AsyncSystem asyncSystem{pMockTaskProcessor};

  auto pMockSubtreeResponse = std::make_unique<SimpleAssetResponse>(
      uint16_t(200),
      "0.subtree",
      CesiumAsync::HttpHeaders{},
      subtreeBytes);

  auto pMockSubtreeRequest = std::make_unique<SimpleAssetRequest>(
      "GET",
      "0.subtree",
      CesiumAsync::HttpHeaders{},
      std::move(pMockSubtreeResponse));

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mapUrlToRequest{
      {"0.subtree", std::move(pMockSubtreeRequest)}};

  auto pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mapUrlToRequest));

  Cesium3DTilesReader::SubtreeFileReader reader;
  CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree> readResult =
      reader.load(asyncSystem, pMockAssetAccessor, "0.subtree")
          .waitInMainThread();

  REQUIRE(readResult.errors.empty());
  REQUIRE(readResult.warnings.empty());
  REQUIRE(readResult.value.has_value());

  Cesium3DTiles::Subtree& readSubtree = readResult.value.value();
  const std::vector<std::byte> readSubtreeBuffer =
      readSubtree.buffers[0].cesium.data;

  REQUIRE(readSubtreeBuffer == bufferData);
  REQUIRE(readSubtree.buffers[0].byteLength == 11);
}
