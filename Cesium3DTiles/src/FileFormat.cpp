#include <Cesium3DTiles/FileFormat.h>

#include <string>

namespace Cesium3DTiles {
/*static*/ std::unordered_map<std::string, FileFormat::Type>
    FileFormat::_magicToType{
        {"glTF", FileFormat::Type::GltfBinary},
        {"b3dm", FileFormat::Type::Batched3dModel},
        {"cmpt", FileFormat::Type::Composite},
        {"i3dm", FileFormat::Type::Instanced3DModel},
        {"pnts", FileFormat::Type::PointCloud},
        {"subt", FileFormat::Type::SubtreeBinary}};

/*static*/ std::unordered_map<FileFormat::Type, std::string>
    FileFormat::_typeToMagic{
        {FileFormat::Type::GltfBinary, "glTF"},
        {FileFormat::Type::Batched3dModel, "b3dm"},
        {FileFormat::Type::Composite, "cmpt"},
        {FileFormat::Type::Instanced3DModel, "i3dm"},
        {FileFormat::Type::PointCloud, "pnts"},
        {FileFormat::Type::SubtreeBinary, "subt"}};

/*static*/ std::string
FileFormat::getMagic(const std::span<const std::byte>& data, size_t length) {
  if (data.size() < length) {
    return std::string();
  }
  return std::string(reinterpret_cast<const char*>(data.data()), length);
}

/*static*/ std::string FileFormat::getMagicForType(Type type) {
  auto magicIt = _typeToMagic.find(type);
  return magicIt != _typeToMagic.end() ? magicIt->second : std::string();
}

/*static*/ FileFormat::Type
FileFormat::getTypeFromMagic(const std::string& magic) {
  auto typeIt = _magicToType.find(magic);
  return typeIt != _magicToType.end() ? typeIt->second : Type::Unknown;
}

/*static*/ FileFormat::Type
FileFormat::getBestFittingFormat(rapidjson::Document& document) {
  if (document.IsNull()) {
    return Type::Unknown;
  }

  if (document.FindMember("root") != document.MemberEnd()) {
    return Type::TilesetJson;
  }

  if (document.FindMember("asset") != document.MemberEnd()) {
    return Type::GltfJson;
  }

  if (document.FindMember("tileAvailability") != document.MemberEnd()) {
    return Type::SubtreeJson;
  }

  auto typeIt = document.FindMember("type");
  if (typeIt != document.MemberEnd() && typeIt->value.IsString()) {
    return Type::GeoJson;
  }

  auto formatIt = document.FindMember("format");
  if (formatIt != document.MemberEnd() && formatIt->value.IsString()) {
    return std::string(formatIt->value.GetString()) == "quantized-mesh-1.0"
               ? Type::LayerJson
               : Type::Unknown;
  }

  return Type::Unknown;
}

/*static*/ FileFormat::Type
FileFormat::getBestFittingFormat(const std::span<const std::byte>& data) {
  return getTypeFromMagic(getMagic(data));
}

} // namespace Cesium3DTiles
