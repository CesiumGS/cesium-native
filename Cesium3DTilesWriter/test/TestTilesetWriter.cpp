#include <Cesium3DTilesReader/TilesetReader.h>
#include <Cesium3DTilesWriter/TilesetWriter.h>

#include <doctest/doctest.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace {
void check(const std::string& input, const std::string& expectedOutput) {
  Cesium3DTilesReader::TilesetReader reader;
  auto readResult = reader.readFromJson(std::span(
      reinterpret_cast<const std::byte*>(input.c_str()),
      input.size()));
  REQUIRE(readResult.errors.empty());
  REQUIRE(readResult.warnings.empty());
  REQUIRE(readResult.value.has_value());

  Cesium3DTiles::Tileset& tileset = readResult.value.value();

  Cesium3DTilesWriter::TilesetWriter writer;
  Cesium3DTilesWriter::TilesetWriterResult writeResult =
      writer.writeTileset(tileset);
  const auto tilesetBytes = writeResult.tilesetBytes;

  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  const std::string tilesetString(
      reinterpret_cast<const char*>(tilesetBytes.data()),
      tilesetBytes.size());

  rapidjson::Document tilesetJson;
  tilesetJson.Parse(tilesetString.c_str());

  rapidjson::Document expectedJson;
  expectedJson.Parse(expectedOutput.c_str());

  REQUIRE(tilesetJson == expectedJson);
}

bool hasSpaces(const std::string& input) {
  return std::count_if(input.begin(), input.end(), [](unsigned char c) {
    return std::isspace(c);
  });
}
} // namespace

TEST_CASE("Writes tileset JSON") {
  std::string string = R"(
    {
      "asset": {
        "version": "1.0",
        "tilesetVersion": "1.2.3"
      },
      "properties": {
        "property1": {
          "maximum": 10.0,
          "minimum": 0.0
        },
        "property2": {
          "maximum": 5.0,
          "minimum": 1.0
        }
      },
      "geometricError": 45.0,
      "root": {
        "boundingVolume": {
          "box": [20.0, 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0]
        },
        "geometricError": 35.0,
        "refine": "REPLACE",
        "children": [
          {
            "boundingVolume": {
              "box": [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
            },
            "geometricError": 15.0,
            "refine": "ADD",
            "content": {
              "uri": "1.gltf"
            }
          },
          {
            "boundingVolume": {
              "box": [10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0]
            },
            "viewerRequestVolume": {
              "box": [30.0, 31.0, 32.0, 33.0, 34.0, 35.0, 36.0, 37.0, 38.0, 39.0, 40.0, 41.0]
            },
            "geometricError": 25.0,
            "content": {
              "boundingVolume": {
                "sphere": [30.0, 31.0, 32.0, 33.0]
              },
              "uri": "2.gltf"
            }
          }
        ]
      }
    }
  )";

  check(string, string);
}

TEST_CASE("Writes tileset JSON with extras") {
  std::string string = R"(
    {
      "asset": {
        "version": "1.0"
      },
      "geometricError": 45.0,
      "root": {
        "boundingVolume": {
          "box": [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
        },
        "geometricError": 15.0,
        "refine": "ADD",
        "extras": {
          "D": "Goodbye"
        }
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

TEST_CASE("Writes tileset JSON with 3DTILES_bounding_volume_S2 extension") {
  std::string string = R"(
    {
      "asset": {
        "version": "1.0"
      },
      "geometricError": 45.0,
      "root": {
        "boundingVolume": {
          "extensions": {
            "3DTILES_bounding_volume_S2": {
              "token": "3",
              "minimumHeight": 0,
              "maximumHeight": 1000000
            }
          }
        },
        "geometricError": 15.0,
        "refine": "ADD",
        "content": {
          "uri": "root.glb"
        }
      },
      "extensionsUsed": [
        "3DTILES_bounding_volume_S2"
      ],
      "extensionsRequired": [
        "3DTILES_bounding_volume_S2"
      ]
    }
  )";

  check(string, string);
}

TEST_CASE("Writes tileset JSON with custom extension") {
  std::string string = R"(
    {
      "asset": {
        "version": "1.0"
      },
      "geometricError": 45.0,
      "root": {
        "boundingVolume": {
          "box": [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
        },
        "geometricError": 15.0,
        "refine": "ADD"
      },
      "extensionsUsed": ["A", "B"],
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

TEST_CASE("Writes tileset JSON with default values removed") {
  std::string string = R"(
    {
      "asset": {
        "version": "1.0"
      },
      "geometricError": 45.0,
      "root": {
        "boundingVolume": {
          "box": [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
        },
        "geometricError": 15.0,
        "refine": "ADD",
        "transform": [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]
      }
    }
  )";

  std::string expected = R"(
    {
      "asset": {
        "version": "1.0"
      },
      "geometricError": 45.0,
      "root": {
        "boundingVolume": {
          "box": [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
        },
        "geometricError": 15.0,
        "refine": "ADD"
      }
    }
  )";

  check(string, expected);
}

TEST_CASE("Writes tileset with prettyPrint") {
  Cesium3DTiles::Tileset tileset;
  tileset.asset.version = "2.0";

  Cesium3DTilesWriter::TilesetWriter writer;
  Cesium3DTilesWriter::TilesetWriterOptions options;
  options.prettyPrint = false;

  Cesium3DTilesWriter::TilesetWriterResult writeResult =
      writer.writeTileset(tileset, options);
  const std::vector<std::byte>& tilesetBytesCompact = writeResult.tilesetBytes;

  std::string tilesetStringCompact(
      reinterpret_cast<const char*>(tilesetBytesCompact.data()),
      tilesetBytesCompact.size());

  REQUIRE_FALSE(hasSpaces(tilesetStringCompact));

  options.prettyPrint = true;
  writeResult = writer.writeTileset(tileset, options);
  const std::vector<std::byte>& tilesetBytesPretty = writeResult.tilesetBytes;
  std::string tilesetStringPretty(
      reinterpret_cast<const char*>(tilesetBytesPretty.data()),
      tilesetBytesPretty.size());

  REQUIRE(hasSpaces(tilesetStringPretty));
}
