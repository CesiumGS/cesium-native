#include <Cesium3DTiles/FileFormat.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace Cesium3DTiles;

namespace {
struct GltfHeader {
  char magic[4];
  uint32_t version;
  uint32_t length;
};

rapidjson::Document createDocumentWithMembers(
    const std::vector<std::pair<std::string, std::string>>& members) {
  rapidjson::Document document;
  document.SetObject();
  for (const auto& [name, value] : members) {
    rapidjson::Value key(name.c_str(), document.GetAllocator());
    rapidjson::Value val(value.c_str(), document.GetAllocator());
    document.AddMember(key, val, document.GetAllocator());
  }
  return document;
}

std::vector<std::byte> asBytes(const std::string& value) {
  std::vector<std::byte> result(value.size());
  std::memcpy(result.data(), value.data(), value.size());
  return result;
}

} // namespace

TEST_CASE("FileFormat::getMagic") {
  SUBCASE("reads the first four bytes by default") {
    using namespace std::string_literals;
    std::string gltf = R"(
      {
        "asset": {
          "version": "2.0"
        }
      }
    )";

    GltfHeader header;
    header.magic[0] = 'g';
    header.magic[1] = 'l';
    header.magic[2] = 'T';
    header.magic[3] = 'F';
    header.version = 2;
    header.length = static_cast<uint32_t>(gltf.size() + sizeof(GltfHeader));

    std::vector<std::byte> data(sizeof(GltfHeader) + gltf.size());
    std::memcpy(data.data(), &header, sizeof(GltfHeader));
    std::memcpy(data.data() + sizeof(GltfHeader), gltf.data(), gltf.size());

    CHECK_EQ(FileFormat::getMagic(data), "glTF");
  }

  SUBCASE("reads a custom length") {
    std::vector<std::byte> data = asBytes("testmagic{asset:{}}");
    CHECK_EQ(FileFormat::getMagic(data, 9), "testmagic");
  }

  SUBCASE("returns an empty string when there are too few bytes") {
    std::vector<std::byte> data = asBytes("glb");
    CHECK_EQ(FileFormat::getMagic(data), std::string());
    CHECK_EQ(FileFormat::getMagic(std::vector<std::byte>()), std::string());
  }
}

TEST_CASE("FileFormat::getMagicForType") {
  CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::GltfBinary), "glTF");
  CHECK_EQ(
      FileFormat::getMagicForType(FileFormat::Type::Batched3dModel),
      "b3dm");
  CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::Composite), "cmpt");
  CHECK_EQ(
      FileFormat::getMagicForType(FileFormat::Type::Instanced3DModel),
      "i3dm");
  CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::PointCloud), "pnts");
  CHECK_EQ(
      FileFormat::getMagicForType(FileFormat::Type::SubtreeBinary),
      "subt");

  SUBCASE("returns an empty string for types without magic") {
    CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::Unknown), "");
    CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::GltfJson), "");
    CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::SubtreeJson), "");
    CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::TilesetJson), "");
    CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::LayerJson), "");
    CHECK_EQ(FileFormat::getMagicForType(FileFormat::Type::GeoJson), "");
  }
}

TEST_CASE("FileFormat::getTypeFromMagic") {
  CHECK_EQ(FileFormat::getTypeFromMagic("glTF"), FileFormat::Type::GltfBinary);
  CHECK_EQ(
      FileFormat::getTypeFromMagic("b3dm"),
      FileFormat::Type::Batched3dModel);
  CHECK_EQ(FileFormat::getTypeFromMagic("cmpt"), FileFormat::Type::Composite);
  CHECK_EQ(
      FileFormat::getTypeFromMagic("i3dm"),
      FileFormat::Type::Instanced3DModel);
  CHECK_EQ(FileFormat::getTypeFromMagic("pnts"), FileFormat::Type::PointCloud);
  CHECK_EQ(
      FileFormat::getTypeFromMagic("subt"),
      FileFormat::Type::SubtreeBinary);

  SUBCASE("returns Unknown for unrecognized magic") {
    CHECK_EQ(FileFormat::getTypeFromMagic("nope"), FileFormat::Type::Unknown);
    CHECK_EQ(FileFormat::getTypeFromMagic(""), FileFormat::Type::Unknown);
    CHECK_EQ(FileFormat::getTypeFromMagic("GLTF"), FileFormat::Type::Unknown);
  }

  SUBCASE("round-trips with getMagicForType") {
    for (FileFormat::Type type :
         {FileFormat::Type::GltfBinary,
          FileFormat::Type::Batched3dModel,
          FileFormat::Type::Composite,
          FileFormat::Type::Instanced3DModel,
          FileFormat::Type::PointCloud,
          FileFormat::Type::SubtreeBinary}) {
      CHECK_EQ(
          FileFormat::getTypeFromMagic(FileFormat::getMagicForType(type)),
          type);
    }
  }
}

TEST_CASE("FileFormat::getBestFittingFormat for JSON") {
  using Members = std::vector<std::pair<std::string, std::string>>;

  SUBCASE("returns Unknown for a null document") {
    rapidjson::Document document;
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::Unknown);
  }

  SUBCASE("returns Unknown for an empty object") {
    rapidjson::Document document = createDocumentWithMembers(Members{});
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::Unknown);
  }

  SUBCASE("returns Unknown for unrelated members") {
    rapidjson::Document document =
        createDocumentWithMembers({{"foo", "a"}, {"bar", "b"}});
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::Unknown);
  }

  SUBCASE("detects a tileset.json from `root`") {
    rapidjson::Document document = createDocumentWithMembers({{"root", "a"}});
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::TilesetJson);
  }

  SUBCASE("detects a glTF from `asset`") {
    rapidjson::Document document = createDocumentWithMembers({{"asset", "a"}});
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::GltfJson);
  }

  SUBCASE("detects a JSON subtree from `tileAvailability`") {
    rapidjson::Document document = createDocumentWithMembers(
        {{"tileAvailability", "childSubtreeAvailability"}});
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::SubtreeJson);
  }

  SUBCASE("detects GeoJSON from a string `type`") {
    rapidjson::Document document =
        createDocumentWithMembers({{"type", "FeatureCollection"}});
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::GeoJson);
  }

  SUBCASE("ignores a non-string `type`") {
    rapidjson::Document document;
    document.SetObject();
    document.AddMember("type", 1, document.GetAllocator());
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::Unknown);
  }

  SUBCASE("detects a quantized-mesh layer.json from `format`") {
    rapidjson::Document document =
        createDocumentWithMembers({{"format", "quantized-mesh-1.0"}});
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::LayerJson);
  }

  SUBCASE("returns Unknown for other `format` values") {
    rapidjson::Document document =
        createDocumentWithMembers({{"format", "something-else"}});
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::Unknown);
  }

  SUBCASE("ignores a non-string `format`") {
    rapidjson::Document document;
    document.SetObject();
    document.AddMember("format", 1, document.GetAllocator());
    CHECK_EQ(
        FileFormat::getBestFittingFormat(document),
        FileFormat::Type::Unknown);
  }
}

TEST_CASE("FileFormat::getBestFittingFormat for binary data") {
  CHECK_EQ(
      FileFormat::getBestFittingFormat(asBytes("glTF....")),
      FileFormat::Type::GltfBinary);
  CHECK_EQ(
      FileFormat::getBestFittingFormat(asBytes("b3dm....")),
      FileFormat::Type::Batched3dModel);
  CHECK_EQ(
      FileFormat::getBestFittingFormat(asBytes("cmpt....")),
      FileFormat::Type::Composite);
  CHECK_EQ(
      FileFormat::getBestFittingFormat(asBytes("i3dm....")),
      FileFormat::Type::Instanced3DModel);
  CHECK_EQ(
      FileFormat::getBestFittingFormat(asBytes("pnts....")),
      FileFormat::Type::PointCloud);
  CHECK_EQ(
      FileFormat::getBestFittingFormat(asBytes("subt....")),
      FileFormat::Type::SubtreeBinary);
  CHECK_EQ(
      FileFormat::getBestFittingFormat(asBytes("{\"a\":1}")),
      FileFormat::Type::Unknown);
  CHECK_EQ(
      FileFormat::getBestFittingFormat(asBytes("ab")),
      FileFormat::Type::Unknown);
  CHECK_EQ(
      FileFormat::getBestFittingFormat(std::vector<std::byte>()),
      FileFormat::Type::Unknown);
}
