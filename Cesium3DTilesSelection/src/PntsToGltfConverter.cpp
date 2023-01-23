#include "PntsToGltfConverter.h"

#include "BatchTableToGltfFeatureMetadata.h"
#include "CesiumGeometry/AxisTransforms.h"
#include "draco/attributes/point_attribute.h"
#include "draco/compression/decode.h"
#include "draco/core/decoder_buffer.h"
#include "draco/point_cloud/point_cloud.h"

#include <CesiumGltf/ExtensionCesiumRTC.h>

#include <rapidjson/document.h>
#include <spdlog/fmt/fmt.h>

#include <functional>

namespace Cesium3DTilesSelection {
namespace {
struct PntsHeader {
  unsigned char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t featureTableJsonByteLength;
  uint32_t featureTableBinaryByteLength;
  uint32_t batchTableJsonByteLength;
  uint32_t batchTableBinaryByteLength;
};

void parsePntsHeader(
    const gsl::span<const std::byte>& pntsBinary,
    PntsHeader& header,
    uint32_t& headerLength,
    GltfConverterResult& result) {
  if (pntsBinary.size() < sizeof(PntsHeader)) {
    result.errors.emplaceError("The PNTS is invalid because it is too small to "
                               "include a PNTS header.");
    return;
  }

  const PntsHeader* pHeader =
      reinterpret_cast<const PntsHeader*>(pntsBinary.data());

  header = *pHeader;
  headerLength = sizeof(PntsHeader);

  if (pHeader->version != 1) {
    result.errors.emplaceError(fmt::format(
        "The PNTS file is version {}, which is unsupported.",
        pHeader->version));
    return;
  }

  if (static_cast<uint32_t>(pntsBinary.size()) < pHeader->byteLength) {
    result.errors.emplaceError(
        "The PNTS is invalid because the total data available is less than the "
        "size specified in its header.");
    return;
  }
}

// The only semantic that can have a variable component type is the
// BATCH_ID semantic. For parsing purposes, all other semantics are
// assigned component type NONE.
enum ComponentType { NONE, BYTE, UNSIGNED_BYTE, UNSIGNED_SHORT, UNSIGNED_INT };

struct PntsSemantic {
  bool existsInFeatureTable = false;
  uint32_t byteOffset = 0;
  ComponentType componentType = ComponentType::NONE;
  bool hasDraco = false;
  uint32_t dracoId = 0;
  draco::PointAttribute* pAttribute = nullptr;
};

struct PntsContent {
  uint32_t pointsLength = 0;
  glm::dvec3 rtcCenter;
  glm::dvec3 quantizedVolumeOffset;
  glm::dvec3 quantizedVolumeScale;
  glm::u8vec4 constantRgba;
  uint32_t batchLength = 0;

  PntsSemantic position;
  PntsSemantic positionQuantized;
  PntsSemantic rgba;
  PntsSemantic rgb;
  PntsSemantic rgb565;
  PntsSemantic normal;
  PntsSemantic normalOct16p;
  PntsSemantic batchId;

  bool hasDraco = false;
  uint32_t dracoByteOffset = 0;
  uint32_t dracoByteLength = 0;

  Cesium3DTilesSelection::ErrorList errors;
};

void parseDracoExtension(
    const rapidjson::Value& dracoExtensionValue,
    PntsContent& parsedContent) {
  const auto propertiesIt = dracoExtensionValue.FindMember("properties");
  if (propertiesIt == dracoExtensionValue.MemberEnd() ||
      !propertiesIt->value.IsObject()) {
    parsedContent.errors.emplaceError(
        "Error parsing Draco compression extension, "
        "no valid properties object found.");
    return;
  }

  const auto byteOffsetIt = dracoExtensionValue.FindMember("byteOffset");
  if (byteOffsetIt == dracoExtensionValue.MemberEnd() ||
      !byteOffsetIt->value.IsUint64()) {
    parsedContent.errors.emplaceError(
        "Error parsing Draco compression extension, "
        "no valid byteOffset found.");
    return;
  }

  const auto byteLengthIt = dracoExtensionValue.FindMember("byteLength");
  if (byteLengthIt == dracoExtensionValue.MemberEnd() ||
      !byteLengthIt->value.IsUint64()) {
    parsedContent.errors.emplaceError(
        "Error parsing Draco compression extension, "
        "no valid byteLength found.");
    return;
  }

  parsedContent.hasDraco = true;
  parsedContent.dracoByteOffset = byteOffsetIt->value.GetUint();
  parsedContent.dracoByteLength = byteLengthIt->value.GetUint();

  const rapidjson::Value& dracoPropertiesValue = propertiesIt->value;

  auto positionDracoIdIt = dracoPropertiesValue.FindMember("POSITION");
  if (positionDracoIdIt != dracoPropertiesValue.MemberEnd() &&
      positionDracoIdIt->value.IsInt()) {
    parsedContent.position.hasDraco = true;
    parsedContent.position.dracoId = positionDracoIdIt->value.GetInt();
  }

  auto rgbaDracoIdIt = dracoPropertiesValue.FindMember("RGBA");
  if (rgbaDracoIdIt != dracoPropertiesValue.MemberEnd() &&
      rgbaDracoIdIt->value.IsInt()) {
    parsedContent.rgba.hasDraco = true;
    parsedContent.rgba.dracoId = rgbaDracoIdIt->value.GetInt();
  }

  auto rgbDracoIdIt = dracoPropertiesValue.FindMember("RGB");
  if (rgbDracoIdIt != dracoPropertiesValue.MemberEnd() &&
      rgbDracoIdIt->value.IsInt()) {
    parsedContent.rgb.hasDraco = true;
    parsedContent.rgb.dracoId = rgbDracoIdIt->value.GetInt();
  }

  auto normalDracoIdIt = dracoPropertiesValue.FindMember("NORMAL");
  if (normalDracoIdIt != dracoPropertiesValue.MemberEnd() &&
      normalDracoIdIt->value.IsInt()) {
    parsedContent.normal.dracoId = normalDracoIdIt->value.GetInt();
  }

  auto batchIdDracoIdIt = dracoPropertiesValue.FindMember("BATCH_ID");
  if (batchIdDracoIdIt != dracoPropertiesValue.MemberEnd() &&
      batchIdDracoIdIt->value.IsInt()) {
    parsedContent.batchId.dracoId = batchIdDracoIdIt->value.GetInt();
  }

  return;
}

bool validateJsonArrayValues(
    const rapidjson::Value& arrayValue,
    uint32_t expectedLength,
    std::function<bool(const rapidjson::Value&)> validate) {
  if (!arrayValue.IsArray()) {
    return false;
  }

  if (arrayValue.Size() != expectedLength) {
    return false;
  }

  for (rapidjson::SizeType i = 0; i < expectedLength; i++) {
    if (!validate(arrayValue[i])) {
      return false;
    }
  }

  return true;
}

void parsePositions(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  const auto positionIt = featureTableJson.FindMember("POSITION");
  if (positionIt != featureTableJson.MemberEnd() &&
      positionIt->value.IsObject()) {
    const auto byteOffsetIt = positionIt->value.FindMember("byteOffset");
    if (byteOffsetIt == positionIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, POSITION semantic does not have "
          "valid byteOffset.");
      return;
    }

    parsedContent.position.existsInFeatureTable = true;
    parsedContent.position.byteOffset = byteOffsetIt->value.GetUint();

    return;
  }

  const auto positionQuantizedIt =
      featureTableJson.FindMember("POSITION_QUANTIZED");
  if (positionQuantizedIt != featureTableJson.MemberEnd() &&
      positionQuantizedIt->value.IsObject()) {
    auto quantizedVolumeOffsetIt =
        featureTableJson.FindMember("QUANTIZED_VOLUME_OFFSET");
    auto quantizedVolumeScaleIt =
        featureTableJson.FindMember("QUANTIZED_VOLUME_SCALE");

    auto isDouble = [](const rapidjson::Value& value) -> bool {
      return value.IsDouble();
    };

    if (quantizedVolumeOffsetIt == featureTableJson.MemberEnd() ||
        !validateJsonArrayValues(quantizedVolumeOffsetIt->value, 3, isDouble)) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, POSITION_QUANTIZED is used but "
          "no valid QUANTIZED_VOLUME_OFFSET semantic was found.");
      return;
    }

    if (quantizedVolumeScaleIt == featureTableJson.MemberEnd() ||
        !validateJsonArrayValues(quantizedVolumeScaleIt->value, 3, isDouble)) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, POSITION_QUANTIZED is used but "
          "no valid QUANTIZED_VOLUME_SCALE semantic was found.");
      return;
    }

    const auto byteOffsetIt =
        positionQuantizedIt->value.FindMember("byteOffset");
    if (byteOffsetIt == positionQuantizedIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, POSITION_QUANTIZED semantic does "
          "not have valid byteOffset.");
      return;
    }

    parsedContent.positionQuantized.existsInFeatureTable = true;
    parsedContent.positionQuantized.byteOffset = byteOffsetIt->value.GetUint();

    auto quantizedVolumeOffset = quantizedVolumeOffsetIt->value.GetArray();
    auto quantizedVolumeScale = quantizedVolumeScaleIt->value.GetArray();

    parsedContent.quantizedVolumeOffset = glm::dvec3(
        quantizedVolumeOffset[0].GetDouble(),
        quantizedVolumeOffset[1].GetDouble(),
        quantizedVolumeOffset[2].GetDouble());

    parsedContent.quantizedVolumeScale = glm::dvec3(
        quantizedVolumeScale[0].GetDouble(),
        quantizedVolumeScale[1].GetDouble(),
        quantizedVolumeScale[2].GetDouble());

    return;
  }

  parsedContent.errors.emplaceError(
      "Error parsing PNTS feature table, no POSITION semantic was found. "
      "One of POSITION or POSITION_QUANTIZED must be defined.");

  return;
}

void parseColors(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  const auto rgbaIt = featureTableJson.FindMember("RGBA");
  if (rgbaIt != featureTableJson.MemberEnd() && rgbaIt->value.IsObject()) {
    const auto byteOffsetIt = rgbaIt->value.FindMember("byteOffset");
    if (byteOffsetIt == rgbaIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, RGBA semantic does not have valid "
          "byteOffset.");
      return;
    }

    parsedContent.rgba.existsInFeatureTable = true;
    parsedContent.rgba.byteOffset = byteOffsetIt->value.GetUint();

    return;
  }

  const auto rgbIt = featureTableJson.FindMember("RGB");
  if (rgbIt != featureTableJson.MemberEnd() && rgbIt->value.IsObject()) {
    const auto byteOffsetIt = rgbIt->value.FindMember("byteOffset");
    if (byteOffsetIt == rgbIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, RGB semantic does not have valid "
          "byteOffset.");
      return;
    }

    parsedContent.rgb.existsInFeatureTable = true;
    parsedContent.rgb.byteOffset = byteOffsetIt->value.GetUint();

    return;
  }

  const auto rgb565It = featureTableJson.FindMember("RGB565");
  if (rgb565It != featureTableJson.MemberEnd() && rgb565It->value.IsObject()) {
    const auto byteOffsetIt = rgb565It->value.FindMember("byteOffset");
    if (byteOffsetIt == rgb565It->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, RGB565 semantic does not have "
          "valid byteOffset.");
      return;
    }

    parsedContent.rgb565.existsInFeatureTable = true;
    parsedContent.rgb565.byteOffset = byteOffsetIt->value.GetUint();

    return;
  }

  auto isUint = [](const rapidjson::Value& value) -> bool {
    return value.IsUint();
  };

  const auto constantRgbaIt = featureTableJson.FindMember("CONSTANT_RGBA");
  if (constantRgbaIt != featureTableJson.MemberEnd() &&
      validateJsonArrayValues(constantRgbaIt->value, 4, isUint)) {
    const rapidjson::Value& arrayValue = constantRgbaIt->value;
    parsedContent.constantRgba = glm::u8vec4(
        arrayValue[0].GetUint(),
        arrayValue[1].GetUint(),
        arrayValue[2].GetUint(),
        arrayValue[3].GetUint());
  }
}

void parseNormals(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  const auto normalIt = featureTableJson.FindMember("NORMAL");
  if (normalIt != featureTableJson.MemberEnd() && normalIt->value.IsObject()) {
    const auto byteOffsetIt = normalIt->value.FindMember("byteOffset");
    if (byteOffsetIt == normalIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, NORMAL semantic does not have "
          "valid byteOffset.");
      return;
    }

    parsedContent.normal.existsInFeatureTable = true;
    parsedContent.normal.byteOffset = byteOffsetIt->value.GetUint();

    return;
  }

  const auto normalOct16pIt = featureTableJson.FindMember("NORMAL_OCT16P");
  if (normalOct16pIt != featureTableJson.MemberEnd() &&
      normalOct16pIt->value.IsObject()) {
    const auto byteOffsetIt = normalOct16pIt->value.FindMember("byteOffset");
    if (byteOffsetIt == normalOct16pIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError("Error parsing PNTS feature table, "
                                        "NORMAL_OCT16P semantic does not have "
                                        "valid byteOffset.");
      return;
    }

    parsedContent.normalOct16p.existsInFeatureTable = true;
    parsedContent.normalOct16p.byteOffset = byteOffsetIt->value.GetUint();
  }
}

void parseBatchIds(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  const auto batchIdIt = featureTableJson.FindMember("BATCH_ID");
  if (batchIdIt != featureTableJson.MemberEnd() &&
      batchIdIt->value.IsObject()) {
    const auto byteOffsetIt = batchIdIt->value.FindMember("byteOffset");
    if (byteOffsetIt == batchIdIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, BATCH_ID semantic does not have "
          "valid byteOffset.");
      return;
    }

    parsedContent.batchId.existsInFeatureTable = true;
    parsedContent.batchId.byteOffset = byteOffsetIt->value.GetUint();

    const auto componentTypeIt = batchIdIt->value.FindMember("componentType");
    if (componentTypeIt != featureTableJson.MemberEnd() &&
        componentTypeIt->value.IsString()) {
      const std::string& componentTypeString =
          componentTypeIt->value.GetString();

      if (componentTypeString == "UNSIGNED_BYTE") {
        parsedContent.batchId.componentType = ComponentType::UNSIGNED_BYTE;
      } else if (componentTypeString == "UNSIGNED_INT") {
        parsedContent.batchId.componentType = ComponentType::UNSIGNED_INT;
      } else {
        parsedContent.batchId.componentType = ComponentType::UNSIGNED_SHORT;
      }
    } else {
      parsedContent.batchId.componentType = ComponentType::UNSIGNED_SHORT;
    }
  }

  const auto batchLengthIt = featureTableJson.FindMember("BATCH_LENGTH");
  if (batchLengthIt != featureTableJson.MemberEnd() &&
      batchLengthIt->value.IsUint()) {
    parsedContent.batchLength = batchLengthIt->value.GetUint();
  } else if (parsedContent.batchId.existsInFeatureTable) {
    parsedContent.errors.emplaceError(
        "Error parsing PNTS feature table, BATCH_ID semantic is present but "
        "no valid BATCH_LENGTH was found.");
  }
}

void parseSemanticsFromFeatureTableJson(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  parsePositions(featureTableJson, parsedContent);
  parseColors(featureTableJson, parsedContent);
  parseNormals(featureTableJson, parsedContent);
  parseBatchIds(featureTableJson, parsedContent);
}

void decodeDraco(
    const gsl::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  if (!parsedContent.hasDraco) {
    return;
  }

  draco::Decoder decoder;
  draco::DecoderBuffer buffer;
  buffer.Init(
      (char*)featureTableBinaryData.data() + parsedContent.dracoByteOffset,
      parsedContent.dracoByteLength);

  draco::StatusOr<std::unique_ptr<draco::PointCloud>> dracoResult =
      decoder.DecodePointCloudFromBuffer(&buffer);

  if (!dracoResult.ok()) {
    parsedContent.errors.emplaceError("Error decoding Draco point cloud.");
    return;
  }

  const std::unique_ptr<draco::PointCloud>& pPointCloud = dracoResult.value();

  if (parsedContent.position.hasDraco) {
    draco::PointAttribute* pPositionAttribute =
        pPointCloud->attribute(parsedContent.position.dracoId);
    if (!pPositionAttribute ||
        pPositionAttribute->data_type() != draco::DT_FLOAT32 ||
        pPositionAttribute->num_components() != 3) {
      parsedContent.errors.emplaceError(
          "Error with decoded Draco point cloud, no valid position attribute.");
      return;
    }

    parsedContent.position.pAttribute = pPositionAttribute;
  }

  // TODO: check for other semantics and metadata
}

rapidjson::Document parseFeatureTableJson(
    const gsl::span<const std::byte>& featureTableJsonData,
    PntsContent& parsedContent) {
  rapidjson::Document document;
  document.Parse(
      reinterpret_cast<const char*>(featureTableJsonData.data()),
      featureTableJsonData.size());
  if (document.HasParseError()) {
    parsedContent.errors.emplaceError(fmt::format(
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        document.GetParseError(),
        document.GetErrorOffset()));
    return document;
  }

  const auto pointsLengthIt = document.FindMember("POINTS_LENGTH");
  if (pointsLengthIt == document.MemberEnd() ||
      !pointsLengthIt->value.IsUint()) {
    parsedContent.errors.emplaceError(
        "Error parsing PNTS feature table, no "
        "valid POINTS_LENGTH property was found.");
    return document;
  }

  parsedContent.pointsLength = pointsLengthIt->value.GetUint();

  if (parsedContent.pointsLength == 0) {
    // This *should* be disallowed by the spec, but it currently isn't.
    // In the future, this can be converted to an error.
    parsedContent.errors.emplaceWarning(
        "The PNTS has a POINTS_LENGTH of zero.");
    return document;
  }

  parseSemanticsFromFeatureTableJson(document, parsedContent);
  if (parsedContent.errors) {
    return document;
  }

  const auto extensionsIt = document.FindMember("extensions");
  if (extensionsIt != document.MemberEnd() && extensionsIt->value.IsObject()) {
    const auto dracoExtensionIt =
        extensionsIt->value.FindMember("3DTILES_draco_point_compression");
    if (dracoExtensionIt != extensionsIt->value.MemberEnd() &&
        dracoExtensionIt->value.IsObject()) {
      const rapidjson::Value& dracoExtensionValue = dracoExtensionIt->value;
      parseDracoExtension(dracoExtensionValue, parsedContent);
      if (parsedContent.errors) {
        return document;
      }
    }
  }

  return document;
}

rapidjson::Document parseBatchTableJson(
    const gsl::span<const std::byte>& batchTableJsonData,
    PntsContent& parsedContent) {
  rapidjson::Document document;
  document.Parse(
      reinterpret_cast<const char*>(batchTableJsonData.data()),
      batchTableJsonData.size());
  if (document.HasParseError()) {
    parsedContent.errors.emplaceWarning(fmt::format(
        "Error when parsing batch table JSON, error code {} at byte "
        "offset "
        "{}. Skip parsing metadata",
        document.GetParseError(),
        document.GetErrorOffset()));
  }
  return document;
}


int32_t
createBufferInGltf(CesiumGltf::Model& gltf, std::vector<std::byte> buffer) {
  size_t bufferId = gltf.buffers.size();
  CesiumGltf::Buffer& gltfBuffer = gltf.buffers.emplace_back();
  gltfBuffer.byteLength = static_cast<int32_t>(buffer.size());
  gltfBuffer.cesium.data = std::move(buffer);

  return static_cast<int32_t>(bufferId);
}

int32_t createBufferViewInGltf(
    CesiumGltf::Model& gltf,
    const int32_t bufferId,
    const int64_t byteLength,
    const int64_t byteStride) {
  size_t bufferViewId = gltf.bufferViews.size();
  CesiumGltf::BufferView& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = bufferId;
  bufferView.byteLength = byteLength;
  bufferView.byteOffset = 0;
  bufferView.byteStride = byteStride;
  bufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  return static_cast<int32_t>(bufferViewId);
}

int32_t createAccessorInGltf(
    CesiumGltf::Model& gltf,
    const int32_t bufferViewId,
    const int32_t componentType,
    const int64_t count,
    const std::string type) {
  size_t accessorId = gltf.accessors.size();
  CesiumGltf::Accessor& accessor = gltf.accessors.emplace_back();
  accessor.bufferView = bufferViewId;
  accessor.byteOffset = 0;
  accessor.componentType = componentType;
  accessor.count = count;
  accessor.type = type;

  return static_cast<int32_t>(accessorId);
}

void createGltfFromFeatureTableData(
    const PntsContent& parsedContent,
    const gsl::span<const std::byte>& featureTableBinaryData,
    GltfConverterResult& result) {
  uint32_t pointsLength = parsedContent.pointsLength;

  size_t positionsByteStride = sizeof(glm::vec3);
  size_t positionsByteLength = pointsLength * positionsByteStride;

  std::vector<std::byte> outPositionsBuffer(positionsByteLength);
  gsl::span<glm::vec3> outPositions(
      reinterpret_cast<glm::vec3*>(outPositionsBuffer.data()),
      pointsLength);

  if (parsedContent.position.existsInFeatureTable) {
    // TODO: check for Draco first
    std::memcpy(
        outPositionsBuffer.data(),
        featureTableBinaryData.data() + parsedContent.position.byteOffset,
        positionsByteLength);
  } else {
    assert(parsedContent.positionQuantized.existsInFeatureTable);
    gsl::span<const glm::u16vec3> quantizedPositions(
        reinterpret_cast<const glm::u16vec3*>(
            featureTableBinaryData.data() +
            parsedContent.positionQuantized.byteOffset),
        pointsLength);

    for (size_t i = 0; i < pointsLength; i++) {
      const glm::vec3 quantizedPosition(
          quantizedPositions[i].x,
          quantizedPositions[i].y,
          quantizedPositions[i].z);

      outPositions[i] = quantizedPosition *
                            glm::vec3(parsedContent.quantizedVolumeScale) /
                            65535.0f +
                        glm::vec3(parsedContent.quantizedVolumeOffset);
    }
  }

  result.model = std::make_optional<CesiumGltf::Model>();
  CesiumGltf::Model& gltf = result.model.value();

  int32_t positionsBufferId =
      createBufferInGltf(gltf, std::move(outPositionsBuffer));
  int32_t positionsBufferViewId = createBufferViewInGltf(
      gltf,
      positionsBufferId,
      static_cast<int64_t>(positionsByteLength),
      static_cast<int64_t>(positionsByteStride));

  int32_t positionAccessorId = createAccessorInGltf(
      gltf,
      positionsBufferViewId,
      CesiumGltf::Accessor::ComponentType::FLOAT,
      int64_t(pointsLength),
      CesiumGltf::Accessor::Type::VEC3);

  // Create a single mesh with a single primitive under a single node.

  size_t meshId = gltf.meshes.size();
  CesiumGltf::Mesh& mesh = gltf.meshes.emplace_back();
  CesiumGltf::MeshPrimitive& primitive = mesh.primitives.emplace_back();
  primitive.mode = CesiumGltf::MeshPrimitive::Mode::POINTS;
  primitive.attributes.emplace(
      "POSITION",
      static_cast<int32_t>(positionAccessorId));
  CesiumGltf::Node& node = gltf.nodes.emplace_back();
  std::memcpy(
      node.matrix.data(),
      &CesiumGeometry::AxisTransforms::Z_UP_TO_Y_UP,
      sizeof(glm::dmat4));
  node.mesh = static_cast<int32_t>(meshId);
}

void convertPntsContentToGltf(
    const gsl::span<const std::byte>& pntsBinary,
    const PntsHeader& header,
    uint32_t headerLength,
    GltfConverterResult& result) {
  if (header.featureTableJsonByteLength > 0 &&
      header.featureTableBinaryByteLength > 0) {
    PntsContent parsedContent;

    const gsl::span<const std::byte> featureTableJsonData =
        pntsBinary.subspan(headerLength, header.featureTableJsonByteLength);

    rapidjson::Document featureTableJson =
        parseFeatureTableJson(featureTableJsonData, parsedContent);
    if (parsedContent.errors) {
      result.errors.merge(parsedContent.errors);
      return;
    }

    // If the 3DTILES_draco_point_compression extension is present,
    // the batch table's binary will be compressed with the feature
    // table's binary. Parse both JSONs first in case the extension is there.
    rapidjson::Document batchTableJson;
    if (header.batchTableJsonByteLength > 0) {
      const int64_t batchTableStart = headerLength +
                                      header.featureTableJsonByteLength +
                                      header.featureTableBinaryByteLength;
      const gsl::span<const std::byte> batchTableJsonData = pntsBinary.subspan(
          static_cast<size_t>(batchTableStart),
          header.batchTableJsonByteLength);

      batchTableJson = parseBatchTableJson(batchTableJsonData, parsedContent);
      if (parsedContent.errors) {
        result.errors.merge(parsedContent.errors);
        return;
      }

      const gsl::span<const std::byte> batchTableBinaryData =
          pntsBinary.subspan(
              static_cast<size_t>(
                  batchTableStart + header.batchTableJsonByteLength),
              header.batchTableBinaryByteLength);
    }

    // when parsing the compressed attributes, convert the json and the binary
    // to something that can be put into the existing batchtable to metadata
    // function

    const gsl::span<const std::byte> featureTableBinaryData =
        pntsBinary.subspan(
            static_cast<size_t>(
                headerLength + header.featureTableJsonByteLength),
            header.featureTableBinaryByteLength);

    createGltfFromFeatureTableData(
        parsedContent,
        featureTableBinaryData,
        result);
  }
}
} // namespace

GltfConverterResult PntsToGltfConverter::convert(
    const gsl::span<const std::byte>& pntsBinary,
    const CesiumGltfReader::GltfReaderOptions& /*options*/) {
  GltfConverterResult result;
  PntsHeader header;
  uint32_t headerLength = 0;
  parsePntsHeader(pntsBinary, header, headerLength, result);
  if (result.errors) {
    return result;
  }

  convertPntsContentToGltf(pntsBinary, header, headerLength, result);
  return result;
}
} // namespace Cesium3DTilesSelection
