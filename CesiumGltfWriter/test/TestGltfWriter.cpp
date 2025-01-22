#include <CesiumGltf/Buffer.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumGltfWriter/GltfWriter.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <doctest/doctest.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {
void check(const std::string& input, const std::string& expectedOutput) {
  CesiumGltfReader::GltfReader reader;
  CesiumGltfReader::GltfReaderResult readResult = reader.readGltf(std::span(
      reinterpret_cast<const std::byte*>(input.c_str()),
      input.size()));
  REQUIRE(readResult.errors.empty());
  REQUIRE(readResult.warnings.empty());
  REQUIRE(readResult.model.has_value());

  CesiumGltf::Model& model = readResult.model.value();

  CesiumGltfWriter::GltfWriter writer;
  CesiumGltfWriter::GltfWriterResult writeResult = writer.writeGltf(model);
  const std::vector<std::byte>& gltfBytes = writeResult.gltfBytes;

  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  const std::string gltfString(
      reinterpret_cast<const char*>(gltfBytes.data()),
      gltfBytes.size());

  rapidjson::Document gltfJson;
  gltfJson.Parse(gltfString.c_str());

  rapidjson::Document expectedJson;
  expectedJson.Parse(expectedOutput.c_str());

  REQUIRE(gltfJson == expectedJson);
}

bool hasSpaces(const std::string& input) {
  return std::count_if(input.begin(), input.end(), [](unsigned char c) {
    return std::isspace(c);
  });
}

struct ExtensionModelTest final : public CesiumUtility::ExtensibleObject {
  static inline constexpr const char* ExtensionName = "PRIVATE_model_test";
};

} // namespace

TEST_CASE("Writes glTF") {
  std::string string = R"(
    {
      "asset": {
        "version": "2.0"
      },
      "scene": 0,
      "scenes": [
        {
          "nodes": [
            0
          ]
        }
      ],
      "nodes": [
        {
          "children": [
            1
          ]
        },
        {
          "mesh": 0
        }
      ],
      "meshes": [
        {
          "primitives": [
            {
              "attributes": {
                "NORMAL": 1,
                "POSITION": 2,
                "TEXCOORD_0": 3
              },
              "indices": 0,
              "material": 0
            }
          ]
        }
      ],
      "accessors": [
        {
          "bufferView": 0,
          "componentType": 5123,
          "count": 36,
          "type": "SCALAR"
        },
        {
          "bufferView": 1,
          "componentType": 5126,
          "count": 24,
          "type": "VEC3"
        },
        {
          "bufferView": 1,
          "byteOffset": 288,
          "componentType": 5126,
          "count": 24,
          "max": [
            0.5,
            0.5,
            0.5
          ],
          "min": [
            -0.5,
            -0.5,
            -0.5
          ],
          "type": "VEC3"
        },
        {
          "bufferView": 2,
          "componentType": 5126,
          "count": 24,
          "type": "VEC2"
        }
      ],
      "materials": [
        {
          "pbrMetallicRoughness": {
            "baseColorTexture": {
              "index": 0
            },
            "metallicFactor": 0
          },
          "occlusionTexture": {
            "index": 1,
            "strength": 0.5
          }
        }
      ],
      "textures": [
        {
          "sampler": 0,
          "source": 0
        }
      ],
      "images": [
        {
          "uri": "BaseColor.png"
        },
        {
          "uri": "Occlusion.png"
        }
      ],
      "samplers": [
        {
          "magFilter": 9729,
          "minFilter": 9986
        }
      ],
      "bufferViews": [
        {
          "buffer": 0,
          "byteOffset": 768,
          "byteLength": 72,
          "target": 34963
        },
        {
          "buffer": 0,
          "byteLength": 576,
          "byteStride": 12,
          "target": 34962
        },
        {
          "buffer": 0,
          "byteOffset": 576,
          "byteLength": 192,
          "byteStride": 8,
          "target": 34962
        }
      ],
      "buffers": [
        {
          "byteLength": 840,
          "uri": "BoxTextured0.bin"
        }
      ]
    }
  )";

  check(string, string);
}

TEST_CASE("Writes glTF with extras") {
  std::string string = R"(
    {
      "asset": {
        "version": "2.0"
      },
      "scene": 0,
      "scenes": [
        {
          "nodes": [0]
        }
      ],
      "nodes": [
        {
          "extras": {
            "D": "Goodbye"
          }
        }
      ],
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

TEST_CASE("Writes glTF with custom extension") {
  std::string string = R"(
    {
      "asset": {
        "version": "2.0"
      },
      "scene": 0,
      "scenes": [
        {
          "nodes": [0]
        }
      ],
      "nodes": [
        { }
      ],
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

TEST_CASE("Writes glTF with default values removed") {
  std::string string = R"(
    {
      "asset": {
        "version": "2.0"
      },
      "scene": 0,
      "scenes": [
        {
          "nodes": [
            0
          ]
        }
      ],
      "nodes": [
        {
          "mesh": 0,
          "matrix": [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]
        }
      ],
      "meshes": [
        {
          "primitives": [
            {
              "attributes": {
                "POSITION": 1
              },
              "mode": 4,
              "indices": 0,
              "material": 0
            }
          ]
        }
      ],
      "accessors": [
        {
          "bufferView": 0,
          "componentType": 5123,
          "count": 36,
          "type": "SCALAR"
        },
        {
          "bufferView": 1,
          "byteOffset": 288,
          "componentType": 5126,
          "count": 24,
          "max": [
            0.5,
            0.5,
            0.5
          ],
          "min": [
            -0.5,
            -0.5,
            -0.5
          ],
          "type": "VEC3"
        }
      ],
      "materials": [
        {
          "pbrMetallicRoughness": {
            "baseColorTexture": {
              "index": 0,
              "texCoord": 0
            },
            "roughnessFactor": 1,
            "metallicFactor": 0
          },
          "emissiveFactor": [0, 0, 0]
        }
      ],
      "textures": [
        {
          "sampler": 0,
          "source": 0
        }
      ],
      "images": [
        {
          "uri": "BaseColor.png"
        }
      ],
      "samplers": [
        {
          "magFilter": 9729,
          "minFilter": 9986,
          "wrapS": 10497,
          "wrapT": 10497
        }
      ],
      "bufferViews": [
        {
          "buffer": 0,
          "byteOffset": 768,
          "byteLength": 72,
          "target": 34963
        },
        {
          "buffer": 0,
          "byteLength": 576,
          "byteStride": 12,
          "target": 34962
        }
      ],
      "buffers": [
        {
          "byteLength": 840,
          "uri": "BoxTextured0.bin"
        }
      ]
    }
  )";

  std::string expected = R"(
    {
      "asset": {
        "version": "2.0"
      },
      "scene": 0,
      "scenes": [
        {
          "nodes": [
            0
          ]
        }
      ],
      "nodes": [
        {
          "mesh": 0
        }
      ],
      "meshes": [
        {
          "primitives": [
            {
              "attributes": {
                "POSITION": 1
              },
              "indices": 0,
              "material": 0
            }
          ]
        }
      ],
      "accessors": [
        {
          "bufferView": 0,
          "componentType": 5123,
          "count": 36,
          "type": "SCALAR"
        },
        {
          "bufferView": 1,
          "byteOffset": 288,
          "componentType": 5126,
          "count": 24,
          "max": [
            0.5,
            0.5,
            0.5
          ],
          "min": [
            -0.5,
            -0.5,
            -0.5
          ],
          "type": "VEC3"
        }
      ],
      "materials": [
        {
          "pbrMetallicRoughness": {
            "baseColorTexture": {
              "index": 0
            },
            "metallicFactor": 0
          }
        }
      ],
      "textures": [
        {
          "sampler": 0,
          "source": 0
        }
      ],
      "images": [
        {
          "uri": "BaseColor.png"
        }
      ],
      "samplers": [
        {
          "magFilter": 9729,
          "minFilter": 9986
        }
      ],
      "bufferViews": [
        {
          "buffer": 0,
          "byteOffset": 768,
          "byteLength": 72,
          "target": 34963
        },
        {
          "buffer": 0,
          "byteLength": 576,
          "byteStride": 12,
          "target": 34962
        }
      ],
      "buffers": [
        {
          "byteLength": 840,
          "uri": "BoxTextured0.bin"
        }
      ]
    }
  )";

  check(string, expected);
}

TEST_CASE("Writes glTF with prettyPrint") {
  CesiumGltf::Model model;
  model.asset.version = "2.0";

  CesiumGltfWriter::GltfWriter writer;
  CesiumGltfWriter::GltfWriterOptions options;
  options.prettyPrint = false;

  CesiumGltfWriter::GltfWriterResult writeResult =
      writer.writeGltf(model, options);
  const std::vector<std::byte>& gltfBytesCompact = writeResult.gltfBytes;

  std::string gltfStringCompact(
      reinterpret_cast<const char*>(gltfBytesCompact.data()),
      gltfBytesCompact.size());

  REQUIRE_FALSE(hasSpaces(gltfStringCompact));

  options.prettyPrint = true;
  writeResult = writer.writeGltf(model, options);
  const std::vector<std::byte>& gltfBytesPretty = writeResult.gltfBytes;
  std::string gltfStringPretty(
      reinterpret_cast<const char*>(gltfBytesPretty.data()),
      gltfBytesPretty.size());

  REQUIRE(hasSpaces(gltfStringPretty));
}

TEST_CASE("Writes glb") {
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

  CesiumGltf::Model model;
  model.asset.version = "2.0";
  CesiumGltf::Buffer buffer;
  buffer.byteLength = static_cast<int64_t>(bufferData.size());
  model.buffers.push_back(buffer);

  CesiumGltfWriter::GltfWriter writer;
  CesiumGltfWriter::GltfWriterResult writeResult =
      writer.writeGlb(model, std::span(bufferData));
  const std::vector<std::byte>& glbBytes = writeResult.gltfBytes;

  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  // Now read the glb back
  CesiumGltfReader::GltfReader reader;
  CesiumGltfReader::GltfReaderResult readResult = reader.readGltf(glbBytes);

  REQUIRE(readResult.errors.empty());
  REQUIRE(readResult.warnings.empty());
  REQUIRE(readResult.model.has_value());

  CesiumGltf::Model& readModel = readResult.model.value();
  const std::vector<std::byte> readModelBuffer =
      readModel.buffers[0].cesium.data;

  REQUIRE(readModelBuffer == bufferData);
  REQUIRE(readModel.asset.version == "2.0");
  REQUIRE(readModel.buffers[0].byteLength == 11);
}

TEST_CASE("Writes glb with binaryChunkByteAlignment of 8") {
  const std::vector<std::byte> bufferData(8);

  CesiumGltf::Model model;
  model.asset.version = "2.0";
  model.asset.generator = "...";

  CesiumGltfWriter::GltfWriter writer;
  CesiumGltfWriter::GltfWriterOptions options;
  options.binaryChunkByteAlignment = 4; // default

  CesiumGltfWriter::GltfWriterResult writeResult =
      writer.writeGlb(model, std::span(bufferData), options);
  const std::vector<std::byte>& glbBytesDefaultPadding = writeResult.gltfBytes;

  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  REQUIRE(glbBytesDefaultPadding.size() == 84);

  options.binaryChunkByteAlignment = 8;
  writeResult = writer.writeGlb(model, std::span(bufferData), options);
  const std::vector<std::byte>& glbBytesExtraPadding = writeResult.gltfBytes;

  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  REQUIRE(glbBytesExtraPadding.size() == 88);
}

TEST_CASE("Reports an error if asked to write a GLB larger than 4GB") {
  CesiumGltf::Model model;
  model.asset.version = "2.0";
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = int64_t(std::numeric_limits<uint32_t>::max()) + 1;

  // Hope you have some extra memory!
  buffer.cesium.data.resize(size_t(buffer.byteLength));

  CesiumGltfWriter::GltfWriter writer;
  CesiumGltfWriter::GltfWriterResult result =
      writer.writeGlb(model, buffer.cesium.data);
  REQUIRE(!result.errors.empty());
  CHECK(result.gltfBytes.empty());
}

TEST_CASE("Handles models with unregistered extension") {
  CesiumGltf::Model model;
  model.addExtension<ExtensionModelTest>();

  SUBCASE("Reports a warning if the extension is enabled") {
    CesiumGltfWriter::GltfWriter writer;
    CesiumGltfWriter::GltfWriterResult result = writer.writeGltf(model);
    REQUIRE(!result.warnings.empty());
  }

  SUBCASE("Does not report a warning if the extension is disabled") {
    CesiumGltfWriter::GltfWriter writer;
    writer.getExtensions().setExtensionState(
        ExtensionModelTest::ExtensionName,
        CesiumJsonWriter::ExtensionState::Disabled);
    CesiumGltfWriter::GltfWriterResult result = writer.writeGltf(model);
    REQUIRE(result.warnings.empty());
  }
}
