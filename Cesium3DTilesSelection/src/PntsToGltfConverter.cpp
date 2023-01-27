#include "PntsToGltfConverter.h"

#include "BatchTableToGltfFeatureMetadata.h"
#include "CesiumGeometry/AxisTransforms.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127 4018 4804)
#endif

#include <draco/attributes/point_attribute.h>
#include <draco/compression/decode.h>
#include <draco/core/decoder_buffer.h>
#include <draco/point_cloud/point_cloud.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/ExtensionKhrMaterialsUnlit.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h>
#include <CesiumUtility/AttributeCompression.h>
#include <CesiumUtility/Math.h>

#include <rapidjson/document.h>
#include <spdlog/fmt/fmt.h>

#include <functional>
#include <limits>

using namespace CesiumGltf;
using namespace CesiumUtility;

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

struct PntsSemantic {
  uint32_t byteOffset = 0;
  std::optional<int32_t> dracoId;
  std::vector<std::byte> data;
};

struct DracoBatchTableValue {

  std::optional<int32_t> dracoId;
  std::vector<std::byte> data;
};

enum PntsColorType { CONSTANT, RGBA, RGB, RGB565 };

struct PntsContent {
  uint32_t pointsLength = 0;
  std::optional<glm::dvec3> rtcCenter;
  std::optional<glm::dvec3> quantizedVolumeOffset;
  std::optional<glm::dvec3> quantizedVolumeScale;
  std::optional<glm::u8vec4> constantRgba;

  PntsSemantic position;
  // required by glTF spec
  glm::vec3 positionMin = glm::vec3(std::numeric_limits<float>::max());
  glm::vec3 positionMax = glm::vec3(std::numeric_limits<float>::lowest());

  bool positionQuantized = false;

  std::optional<PntsSemantic> color;
  PntsColorType colorType = PntsColorType::CONSTANT;

  std::optional<PntsSemantic> normal;
  bool normalOctEncoded = false;

  std::optional<PntsSemantic> batchId;
  std::optional<int32_t> batchIdComponentType;

  std::optional<uint32_t> dracoByteOffset;
  std::optional<uint32_t> dracoByteLength;

  std::unordered_map<std::string, PntsSemantic> dracoBatchTableValues;

  Cesium3DTilesSelection::ErrorList errors;
};

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

void parsePositionsFromFeatureTableJson(
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

    auto isNumber = [](const rapidjson::Value& value) -> bool {
      return value.IsNumber();
    };

    if (quantizedVolumeOffsetIt == featureTableJson.MemberEnd() ||
        !validateJsonArrayValues(quantizedVolumeOffsetIt->value, 3, isNumber)) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, POSITION_QUANTIZED is used but "
          "no valid QUANTIZED_VOLUME_OFFSET semantic was found.");
      return;
    }

    if (quantizedVolumeScaleIt == featureTableJson.MemberEnd() ||
        !validateJsonArrayValues(quantizedVolumeScaleIt->value, 3, isNumber)) {
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

    parsedContent.positionQuantized = true;
    parsedContent.position.byteOffset = byteOffsetIt->value.GetUint();

    auto quantizedVolumeOffset = quantizedVolumeOffsetIt->value.GetArray();
    auto quantizedVolumeScale = quantizedVolumeScaleIt->value.GetArray();

    parsedContent.quantizedVolumeOffset = std::make_optional<glm::dvec3>(
        quantizedVolumeOffset[0].GetDouble(),
        quantizedVolumeOffset[1].GetDouble(),
        quantizedVolumeOffset[2].GetDouble());

    parsedContent.quantizedVolumeScale = std::make_optional<glm::dvec3>(
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

void parseColorsFromFeatureTableJson(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  const auto rgbaIt = featureTableJson.FindMember("RGBA");
  if (rgbaIt != featureTableJson.MemberEnd() && rgbaIt->value.IsObject()) {
    const auto byteOffsetIt = rgbaIt->value.FindMember("byteOffset");
    if (byteOffsetIt != rgbaIt->value.MemberEnd() &&
        byteOffsetIt->value.IsUint()) {
      parsedContent.color = std::make_optional<PntsSemantic>();
      parsedContent.color.value().byteOffset = byteOffsetIt->value.GetUint();
      parsedContent.colorType = PntsColorType::RGBA;
      return;
    }

    parsedContent.errors.emplaceWarning(
        "Error parsing PNTS feature table, RGBA semantic does not have valid "
        "byteOffset. Skip parsing RGBA colors.");
  }

  const auto rgbIt = featureTableJson.FindMember("RGB");
  if (rgbIt != featureTableJson.MemberEnd() && rgbIt->value.IsObject()) {
    const auto byteOffsetIt = rgbIt->value.FindMember("byteOffset");
    if (byteOffsetIt != rgbIt->value.MemberEnd() &&
        byteOffsetIt->value.IsUint()) {
      parsedContent.color = std::make_optional<PntsSemantic>();
      parsedContent.color.value().byteOffset = byteOffsetIt->value.GetUint();
      parsedContent.colorType = PntsColorType::RGB;
      return;
    }

    parsedContent.errors.emplaceWarning(
        "Error parsing PNTS feature table, RGB semantic does not have valid "
        "byteOffset. Skip parsing RGB colors.");
  }

  const auto rgb565It = featureTableJson.FindMember("RGB565");
  if (rgb565It != featureTableJson.MemberEnd() && rgb565It->value.IsObject()) {
    const auto byteOffsetIt = rgb565It->value.FindMember("byteOffset");
    if (byteOffsetIt != rgb565It->value.MemberEnd() &&
        byteOffsetIt->value.IsUint()) {
      parsedContent.color = std::make_optional<PntsSemantic>();
      parsedContent.color.value().byteOffset = byteOffsetIt->value.GetUint();
      parsedContent.colorType = PntsColorType::RGB565;
      return;
    }

    parsedContent.errors.emplaceWarning(
        "Error parsing PNTS feature table, RGB565 semantic does not have "
        "valid byteOffset. Skip parsing RGB565 colors.");
  }

  auto isUint = [](const rapidjson::Value& value) -> bool {
    return value.IsUint();
  };

  const auto constantRgbaIt = featureTableJson.FindMember("CONSTANT_RGBA");
  if (constantRgbaIt != featureTableJson.MemberEnd() &&
      validateJsonArrayValues(constantRgbaIt->value, 4, isUint)) {
    const rapidjson::Value& arrayValue = constantRgbaIt->value;
    parsedContent.constantRgba = std::make_optional<glm::u8vec4>(
        arrayValue[0].GetUint(),
        arrayValue[1].GetUint(),
        arrayValue[2].GetUint(),
        arrayValue[3].GetUint());
  }
}

void parseNormalsFromFeatureTableJson(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  const auto normalIt = featureTableJson.FindMember("NORMAL");
  if (normalIt != featureTableJson.MemberEnd() && normalIt->value.IsObject()) {
    const auto byteOffsetIt = normalIt->value.FindMember("byteOffset");
    if (byteOffsetIt != normalIt->value.MemberEnd() &&
        byteOffsetIt->value.IsUint()) {
      parsedContent.normal = std::make_optional<PntsSemantic>();
      parsedContent.normal.value().byteOffset = byteOffsetIt->value.GetUint();
      return;
    }

    parsedContent.errors.emplaceWarning(
        "Error parsing PNTS feature table, NORMAL semantic does not have "
        "valid byteOffset. Skip parsing normals.");
  }

  const auto normalOct16pIt = featureTableJson.FindMember("NORMAL_OCT16P");
  if (normalOct16pIt != featureTableJson.MemberEnd() &&
      normalOct16pIt->value.IsObject()) {
    const auto byteOffsetIt = normalOct16pIt->value.FindMember("byteOffset");
    if (byteOffsetIt != normalOct16pIt->value.MemberEnd() &&
        byteOffsetIt->value.IsUint()) {
      parsedContent.normal = std::make_optional<PntsSemantic>();
      parsedContent.normal.value().byteOffset = byteOffsetIt->value.GetUint();
      parsedContent.normalOctEncoded = true;
      return;
    }

    parsedContent.errors.emplaceWarning(
        "Error parsing PNTS feature table, NORMAL_OCT16P semantic does not "
        "have valid byteOffset. Skip parsing oct-encoded normals");
  }
}

void parseBatchIdsFromFeatureTableJson(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  const auto batchIdIt = featureTableJson.FindMember("BATCH_ID");
  if (batchIdIt != featureTableJson.MemberEnd() &&
      batchIdIt->value.IsObject()) {
    const auto byteOffsetIt = batchIdIt->value.FindMember("byteOffset");
    if (byteOffsetIt == batchIdIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceWarning(
          "Error parsing PNTS feature table, BATCH_ID semantic does not have "
          "valid byteOffset. Skip parsing batch IDs.");
      return;
    }

    parsedContent.batchId = std::make_optional<PntsSemantic>();
    PntsSemantic& batchId = parsedContent.batchId.value();
    batchId.byteOffset = byteOffsetIt->value.GetUint();

    const auto componentTypeIt = batchIdIt->value.FindMember("componentType");
    if (componentTypeIt != featureTableJson.MemberEnd() &&
        componentTypeIt->value.IsString()) {
      const std::string& componentTypeString =
          componentTypeIt->value.GetString();

      if (componentTypeString == "UNSIGNED_BYTE") {
        parsedContent.batchIdComponentType =
            Accessor::ComponentType::UNSIGNED_BYTE;
      } else if (componentTypeString == "UNSIGNED_INT") {
        parsedContent.batchIdComponentType =
            Accessor::ComponentType::UNSIGNED_INT;
      } else {
        parsedContent.batchIdComponentType =
            Accessor::ComponentType::UNSIGNED_SHORT;
      }
    } else {
      parsedContent.batchIdComponentType =
          Accessor::ComponentType::UNSIGNED_SHORT;
    }
  }
}

void parseSemanticsFromFeatureTableJson(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  parsePositionsFromFeatureTableJson(featureTableJson, parsedContent);
  if (parsedContent.errors) {
    return;
  }

  parseColorsFromFeatureTableJson(featureTableJson, parsedContent);
  if (parsedContent.errors) {
    return;
  }

  parseNormalsFromFeatureTableJson(featureTableJson, parsedContent);
  if (parsedContent.errors) {
    return;
  }

  parseBatchIdsFromFeatureTableJson(featureTableJson, parsedContent);
  if (parsedContent.errors) {
    return;
  }

  auto isNumber = [](const rapidjson::Value& value) -> bool {
    return value.IsNumber();
  };

  const auto rtcIt = featureTableJson.FindMember("RTC_CENTER");
  if (rtcIt != featureTableJson.MemberEnd() &&
      validateJsonArrayValues(rtcIt->value, 3, isNumber)) {
    const rapidjson::Value& rtcValue = rtcIt->value;
    parsedContent.rtcCenter = std::make_optional<glm::vec3>(
        rtcValue[0].GetDouble(),
        rtcValue[1].GetDouble(),
        rtcValue[2].GetDouble());
  }
}

void parseDracoExtensionFromFeatureTableJson(
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

  parsedContent.dracoByteOffset =
      std::make_optional<uint32_t>(byteOffsetIt->value.GetUint());
  parsedContent.dracoByteLength =
      std::make_optional<uint32_t>(byteLengthIt->value.GetUint());

  const rapidjson::Value& dracoPropertiesValue = propertiesIt->value;

  auto positionDracoIdIt = dracoPropertiesValue.FindMember("POSITION");
  if (positionDracoIdIt != dracoPropertiesValue.MemberEnd() &&
      positionDracoIdIt->value.IsInt()) {
    parsedContent.position.dracoId =
        std::make_optional<int32_t>(positionDracoIdIt->value.GetInt());
  }

  if (parsedContent.color) {
    const PntsColorType& colorType = parsedContent.colorType;
    if (colorType == PntsColorType::RGBA) {
      auto rgbaDracoIdIt = dracoPropertiesValue.FindMember("RGBA");
      if (rgbaDracoIdIt != dracoPropertiesValue.MemberEnd() &&
          rgbaDracoIdIt->value.IsInt()) {
        parsedContent.color.value().dracoId =
            std::make_optional<int32_t>(rgbaDracoIdIt->value.GetInt());
      }
    } else if (colorType == PntsColorType::RGB) {
      auto rgbDracoIdIt = dracoPropertiesValue.FindMember("RGB");
      if (rgbDracoIdIt != dracoPropertiesValue.MemberEnd() &&
          rgbDracoIdIt->value.IsInt()) {
        parsedContent.color.value().dracoId =
            std::make_optional<int32_t>(rgbDracoIdIt->value.GetInt());
      }
    }
  }

  if (parsedContent.normal) {
    auto normalDracoIdIt = dracoPropertiesValue.FindMember("NORMAL");
    if (normalDracoIdIt != dracoPropertiesValue.MemberEnd() &&
        normalDracoIdIt->value.IsInt()) {
      parsedContent.normal.value().dracoId =
          std::make_optional<int32_t>(normalDracoIdIt->value.GetInt());
    }
  }

  if (parsedContent.batchId) {
    auto batchIdDracoIdIt = dracoPropertiesValue.FindMember("BATCH_ID");
    if (batchIdDracoIdIt != dracoPropertiesValue.MemberEnd() &&
        batchIdDracoIdIt->value.IsInt()) {
      parsedContent.batchId.value().dracoId =
          std::make_optional<int32_t>(batchIdDracoIdIt->value.GetInt());
    }
  }
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
      parseDracoExtensionFromFeatureTableJson(
          dracoExtensionValue,
          parsedContent);
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
    return document;
  }

  const auto extensionsIt = document.FindMember("extensions");
  if (extensionsIt != document.MemberEnd() && extensionsIt->value.IsObject()) {
    const auto dracoExtensionIt =
        extensionsIt->value.FindMember("3DTILES_draco_point_compression");
    if (dracoExtensionIt != extensionsIt->value.MemberEnd() &&
        dracoExtensionIt->value.IsObject()) {
      const rapidjson::Value& dracoExtensionValue = dracoExtensionIt->value;
      auto& dracoBatchTableValues = parsedContent.dracoBatchTableValues;
      for (auto propertyIt = dracoExtensionValue.MemberBegin();
           propertyIt != dracoExtensionValue.MemberEnd();
           ++propertyIt) {
        std::string name = propertyIt->name.GetString();
        if (propertyIt->value.IsInt()) {
          PntsSemantic metadataSemantic;
          metadataSemantic.dracoId = propertyIt->value.GetInt();
          dracoBatchTableValues.insert(std::make_pair(name, metadataSemantic));
        } else {
          parsedContent.errors.emplaceWarning(
              "Error parsing metadata property " + name +
              " due to invalid Draco ID. The values for this property will not "
              "be parsed properly.");
        }
      }
    }
  }

  return document;
}

bool validateDracoAttribute(
    const draco::PointAttribute* const pAttribute,
    const draco::DataType expectedDataType,
    const int32_t expectedNumComponents) {
  return pAttribute && pAttribute->data_type() == expectedDataType &&
         pAttribute->num_components() == expectedNumComponents;
}

void decodeDraco(
    const gsl::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  if (!parsedContent.dracoByteOffset || !parsedContent.dracoByteLength) {
    return;
  }

  draco::Decoder decoder;
  draco::DecoderBuffer buffer;
  buffer.Init(
      (char*)featureTableBinaryData.data() +
          parsedContent.dracoByteOffset.value(),
      parsedContent.dracoByteLength.value());

  draco::StatusOr<std::unique_ptr<draco::PointCloud>> dracoResult =
      decoder.DecodePointCloudFromBuffer(&buffer);

  if (!dracoResult.ok()) {
    parsedContent.errors.emplaceError("Error decoding Draco point cloud.");
    return;
  }

  const std::unique_ptr<draco::PointCloud>& pPointCloud = dracoResult.value();
  const uint32_t pointsLength = parsedContent.pointsLength;

  if (parsedContent.position.dracoId) {
    draco::PointAttribute* pPositionAttribute =
        pPointCloud->attribute(parsedContent.position.dracoId.value());
    if (!validateDracoAttribute(pPositionAttribute, draco::DT_FLOAT32, 3)) {
      parsedContent.errors.emplaceError(
          "Error with decoded Draco point cloud, no valid position "
          "attribute.");
      return;
    }

    std::vector<std::byte>& positionData = parsedContent.position.data;
    positionData.resize(pointsLength * sizeof(glm::vec3));

    gsl::span<glm::vec3> outPositions(
        reinterpret_cast<glm::vec3*>(positionData.data()),
        pointsLength);

    draco::DataBuffer* decodedBuffer = pPositionAttribute->buffer();
    int64_t decodedByteOffset = pPositionAttribute->byte_offset();
    int64_t decodedByteStride = pPositionAttribute->byte_stride();

    for (uint32_t i = 0; i < pointsLength; ++i) {
      const glm::vec3 position = *reinterpret_cast<const glm::vec3*>(
          decodedBuffer->data() + decodedByteOffset + decodedByteStride * i);
      outPositions[i] = position;

      parsedContent.positionMin = glm::min(position, parsedContent.positionMin);
      parsedContent.positionMax = glm::max(position, parsedContent.positionMax);
    }
  }

  if (parsedContent.color) {
    PntsSemantic& color = parsedContent.color.value();
    if (color.dracoId) {
      std::vector<std::byte>& colorData = color.data;
      draco::PointAttribute* pColorAttribute =
          pPointCloud->attribute(color.dracoId.value());
      if (parsedContent.colorType == PntsColorType::RGBA &&
          validateDracoAttribute(pColorAttribute, draco::DT_UINT8, 4)) {
        colorData.resize(pointsLength * sizeof(glm::u8vec4));

        gsl::span<glm::u8vec4> outColors(
            reinterpret_cast<glm::u8vec4*>(colorData.data()),
            pointsLength);

        draco::DataBuffer* decodedBuffer = pColorAttribute->buffer();
        int64_t decodedByteOffset = pColorAttribute->byte_offset();
        int64_t decodedByteStride = pColorAttribute->byte_stride();

        for (uint32_t i = 0; i < pointsLength; ++i) {
          outColors[i] = *reinterpret_cast<const glm::u8vec4*>(
              decodedBuffer->data() + decodedByteOffset +
              decodedByteStride * i);
        }
      } else if (
          parsedContent.colorType == PntsColorType::RGB &&
          validateDracoAttribute(pColorAttribute, draco::DT_UINT8, 3)) {
        colorData.resize(pointsLength * sizeof(glm::u8vec3));
        gsl::span<glm::u8vec3> outColors(
            reinterpret_cast<glm::u8vec3*>(colorData.data()),
            pointsLength);

        draco::DataBuffer* decodedBuffer = pColorAttribute->buffer();
        int64_t decodedByteOffset = pColorAttribute->byte_offset();
        int64_t decodedByteStride = pColorAttribute->byte_stride();

        for (uint32_t i = 0; i < pointsLength; ++i) {
          outColors[i] = *reinterpret_cast<const glm::u8vec3*>(
              decodedBuffer->data() + decodedByteOffset +
              decodedByteStride * i);
        }
      } else {
        parsedContent.errors.emplaceWarning(
            "Warning: decoded Draco point cloud did not contain a valid "
            "color "
            "attribute. Skip parsing colors.");
        parsedContent.color = std::nullopt;
        parsedContent.colorType = PntsColorType::CONSTANT;
      }
    }
  }

  if (parsedContent.normal) {
    PntsSemantic& normal = parsedContent.normal.value();
    draco::PointAttribute* pNormalAttribute =
        pPointCloud->attribute(normal.dracoId.value());
    if (validateDracoAttribute(pNormalAttribute, draco::DT_FLOAT32, 3)) {
      std::vector<std::byte>& normalData = normal.data;
      normalData.resize(pointsLength * sizeof(glm::vec3));

      gsl::span<glm::vec3> outNormals(
          reinterpret_cast<glm::vec3*>(normalData.data()),
          pointsLength);

      draco::DataBuffer* decodedBuffer = pNormalAttribute->buffer();
      int64_t decodedByteOffset = pNormalAttribute->byte_offset();
      int64_t decodedByteStride = pNormalAttribute->byte_stride();

      for (uint32_t i = 0; i < pointsLength; ++i) {
        outNormals[i] = *reinterpret_cast<const glm::vec3*>(
            decodedBuffer->data() + decodedByteOffset + decodedByteStride * i);
      }
    } else {
      parsedContent.errors.emplaceWarning(
          "Warning: decoded Draco point cloud did not contain valid normal "
          "attribute. Skip parsing normals.");
      parsedContent.normal = std::nullopt;
    }
  }

  if (parsedContent.batchId) {
    PntsSemantic& batchId = parsedContent.batchId.value();
    if (batchId.dracoId) {
      draco::PointAttribute* pBatchIdAttribute =
          pPointCloud->attribute(batchId.dracoId.value());
      std::vector<std::byte>& batchIdData = batchId.data;

      int32_t componentType = 0;
      if (parsedContent.batchIdComponentType) {
        componentType = parsedContent.batchIdComponentType.value();
      }

      if (componentType == Accessor::ComponentType::UNSIGNED_BYTE &&
          validateDracoAttribute(pBatchIdAttribute, draco::DT_UINT8, 1)) {
        batchIdData.resize(pointsLength * sizeof(uint8_t));
        gsl::span<uint8_t> outBatchIds(
            reinterpret_cast<uint8_t*>(batchIdData.data()),
            pointsLength);

        draco::DataBuffer* decodedBuffer = pBatchIdAttribute->buffer();
        int64_t decodedByteOffset = pBatchIdAttribute->byte_offset();
        int64_t decodedByteStride = pBatchIdAttribute->byte_stride();

        for (uint32_t i = 0; i < pointsLength; ++i) {
          outBatchIds[i] = *reinterpret_cast<const uint8_t*>(
              decodedBuffer->data() + decodedByteOffset +
              decodedByteStride * i);
        }
      } else if (
          componentType == Accessor::ComponentType::UNSIGNED_INT &&
          validateDracoAttribute(pBatchIdAttribute, draco::DT_UINT32, 1)) {
        batchIdData.resize(pointsLength * sizeof(uint32_t));
        gsl::span<uint32_t> outBatchIds(
            reinterpret_cast<uint32_t*>(batchIdData.data()),
            pointsLength);

        draco::DataBuffer* decodedBuffer = pBatchIdAttribute->buffer();
        int64_t decodedByteOffset = pBatchIdAttribute->byte_offset();
        int64_t decodedByteStride = pBatchIdAttribute->byte_stride();

        for (uint32_t i = 0; i < pointsLength; ++i) {
          outBatchIds[i] = *reinterpret_cast<const uint32_t*>(
              decodedBuffer->data() + decodedByteOffset +
              decodedByteStride * i);
        }
      } else if (
          (componentType == 0 ||
           componentType == Accessor::ComponentType::UNSIGNED_SHORT) &&
          validateDracoAttribute(pBatchIdAttribute, draco::DT_UINT16, 1)) {
        batchIdData.resize(pointsLength * sizeof(uint16_t));
        gsl::span<uint16_t> outBatchIds(
            reinterpret_cast<uint16_t*>(batchIdData.data()),
            pointsLength);

        draco::DataBuffer* decodedBuffer = pBatchIdAttribute->buffer();
        int64_t decodedByteOffset = pBatchIdAttribute->byte_offset();
        int64_t decodedByteStride = pBatchIdAttribute->byte_stride();

        for (uint32_t i = 0; i < pointsLength; ++i) {
          outBatchIds[i] = *reinterpret_cast<const uint16_t*>(
              decodedBuffer->data() + decodedByteOffset +
              decodedByteStride * i);
        }
      } else {
        parsedContent.errors.emplaceWarning(
            "Warning: decoded Draco point cloud did not contain a valid "
            "batch "
            "id "
            "attribute. Skip parsing batch IDs.");
        parsedContent.batchId = std::nullopt;
      }
    }
  }

  // TODO: check for metadata
}

void parsePositionsFromFeatureTableBinary(
    const gsl::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  std::vector<std::byte>& positionData = parsedContent.position.data;
  if (positionData.size() > 0) {
    // If data isn't empty, it must have been decoded from Draco.
    return;
  }

  const uint32_t pointsLength = parsedContent.pointsLength;
  const size_t positionsByteStride = sizeof(glm::vec3);
  const size_t positionsByteLength = pointsLength * positionsByteStride;
  positionData.resize(positionsByteLength);

  gsl::span<glm::vec3> outPositions(
      reinterpret_cast<glm::vec3*>(positionData.data()),
      pointsLength);

  if (parsedContent.positionQuantized) {
    // PERFORMANCE_IDEA: In the future, it might be more performant to detect
    // if the recipient rendering engine can handle dequantization on its own
    // and if so, use the KHR_mesh_quantization extension to avoid
    // dequantizing here.
    const gsl::span<const glm::u16vec3> quantizedPositions(
        reinterpret_cast<const glm::u16vec3*>(
            featureTableBinaryData.data() + parsedContent.position.byteOffset),
        pointsLength);

    const glm::vec3 quantizedVolumeScale =
        glm::vec3(parsedContent.quantizedVolumeScale.value());
    const glm::vec3 quantizedVolumeOffset =
        glm::vec3(parsedContent.quantizedVolumeOffset.value());

    const glm::vec3 quantizedPositionScalar = quantizedVolumeScale / 65535.0f;

    for (size_t i = 0; i < pointsLength; i++) {
      const glm::vec3 quantizedPosition(
          quantizedPositions[i].x,
          quantizedPositions[i].y,
          quantizedPositions[i].z);

      const glm::vec3 dequantizedPosition =
          quantizedPosition * quantizedPositionScalar + quantizedVolumeOffset;
      outPositions[i] = dequantizedPosition;
      parsedContent.positionMin =
          glm::min(parsedContent.positionMin, dequantizedPosition);
      parsedContent.positionMax =
          glm::max(parsedContent.positionMax, dequantizedPosition);
    }
  } else {
    // The position accessor min / max is required by the glTF spec, so
    // a for loop is used instead of std::memcpy.
    const gsl::span<const glm::vec3> positions(
        reinterpret_cast<const glm::vec3*>(
            featureTableBinaryData.data() + parsedContent.position.byteOffset),
        pointsLength);
    for (size_t i = 0; i < pointsLength; i++) {
      const glm::vec3 position = positions[i];
      outPositions[i] = position;
      parsedContent.positionMin = glm::min(parsedContent.positionMin, position);
      parsedContent.positionMax = glm::max(parsedContent.positionMax, position);
    }
  }
}

void parseColorsFromFeatureTableBinary(
    const gsl::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  PntsSemantic& color = parsedContent.color.value();
  std::vector<std::byte>& colorData = color.data;
  if (colorData.size() > 0) {
    // If data isn't empty, it must have been decoded from Draco.
    return;
  }

  const uint32_t pointsLength = parsedContent.pointsLength;

  if (parsedContent.colorType == PntsColorType::RGBA) {
    const size_t colorsByteStride = sizeof(glm::u8vec4);
    const size_t colorsByteLength = pointsLength * colorsByteStride;
    colorData.resize(colorsByteLength);

    gsl::span<glm::u8vec4> outColors(
        reinterpret_cast<glm::u8vec4*>(colorData.data()),
        pointsLength);

    std::memcpy(
        colorData.data(),
        featureTableBinaryData.data() + color.byteOffset,
        colorsByteLength);
  } else if (parsedContent.colorType == PntsColorType::RGB) {
    const size_t colorsByteStride = sizeof(glm::u8vec3);
    const size_t colorsByteLength = pointsLength * colorsByteStride;
    colorData.resize(colorsByteLength);

    std::memcpy(
        colorData.data(),
        featureTableBinaryData.data() + color.byteOffset,
        colorsByteLength);
  } else if (parsedContent.colorType == PntsColorType::RGB565) {
    const size_t colorsByteStride = sizeof(glm::vec3);
    const size_t colorsByteLength = pointsLength * colorsByteStride;
    colorData.resize(colorsByteLength);

    gsl::span<glm::vec3> outColors(
        reinterpret_cast<glm::vec3*>(colorData.data()),
        pointsLength);

    const gsl::span<const uint16_t> compressedColors(
        reinterpret_cast<const uint16_t*>(
            featureTableBinaryData.data() + color.byteOffset),
        pointsLength);

    for (size_t i = 0; i < pointsLength; i++) {
      const uint16_t compressedColor = compressedColors[i];
      outColors[i] =
          glm::vec3(AttributeCompression::decodeRGB565(compressedColor));
    }
  }
}

void parseNormalsFromFeatureTableBinary(
    const gsl::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  PntsSemantic& normal = parsedContent.normal.value();
  std::vector<std::byte>& normalData = normal.data;
  if (normalData.size() > 0) {
    // If data isn't empty, it must have been decoded from Draco.
    return;
  }

  const uint32_t pointsLength = parsedContent.pointsLength;
  const size_t normalsByteStride = sizeof(glm::vec3);
  const size_t normalsByteLength = pointsLength * normalsByteStride;
  normalData.resize(normalsByteLength);

  if (parsedContent.normalOctEncoded) {
    const gsl::span<const glm::u8vec2> encodedNormals(
        reinterpret_cast<const glm::u8vec2*>(
            featureTableBinaryData.data() + normal.byteOffset),
        pointsLength);

    gsl::span<glm::vec3> outNormals(
        reinterpret_cast<glm::vec3*>(normalData.data()),
        pointsLength);

    for (size_t i = 0; i < pointsLength; i++) {
      const glm::u8vec2 encodedNormal = encodedNormals[i];
      outNormals[i] = glm::vec3(CesiumUtility::AttributeCompression::octDecode(
          encodedNormal.x,
          encodedNormal.y));
    }
  } else {
    std::memcpy(
        normalData.data(),
        featureTableBinaryData.data() + normal.byteOffset,
        normalsByteLength);
  }
}

void parseBatchIdsFromFeatureTableBinary(
    const gsl::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  PntsSemantic& batchId = parsedContent.batchId.value();
  std::vector<std::byte>& batchIdData = batchId.data;
  if (batchIdData.size() > 0) {
    // If data isn't empty, it must have been decoded from Draco.
    return;
  }

  const uint32_t pointsLength = parsedContent.pointsLength;
  size_t batchIdsByteStride = sizeof(uint16_t);
  if (parsedContent.batchIdComponentType) {
    int8_t componentByteSize = Accessor::computeByteSizeOfComponent(
        parsedContent.batchIdComponentType.value());
    batchIdsByteStride = static_cast<size_t>(componentByteSize);
  }
  const size_t batchIdsByteLength = pointsLength * batchIdsByteStride;
  batchIdData.resize(batchIdsByteLength);

  std::memcpy(
      batchIdData.data(),
      featureTableBinaryData.data() + batchId.byteOffset,
      batchIdsByteLength);
}

void parseFeatureTableBinary(
    const gsl::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  decodeDraco(featureTableBinaryData, parsedContent);
  parsePositionsFromFeatureTableBinary(featureTableBinaryData, parsedContent);
  if (parsedContent.color) {
    parseColorsFromFeatureTableBinary(featureTableBinaryData, parsedContent);
  }
  if (parsedContent.normal) {
    parseNormalsFromFeatureTableBinary(featureTableBinaryData, parsedContent);
  }
  if (parsedContent.batchId) {
    parseBatchIdsFromFeatureTableBinary(featureTableBinaryData, parsedContent);
  }
}

int32_t createBufferInGltf(Model& gltf, std::vector<std::byte>& buffer) {
  size_t bufferId = gltf.buffers.size();
  Buffer& gltfBuffer = gltf.buffers.emplace_back();
  gltfBuffer.byteLength = static_cast<int32_t>(buffer.size());
  gltfBuffer.cesium.data = std::move(buffer);

  return static_cast<int32_t>(bufferId);
}

int32_t createBufferViewInGltf(
    Model& gltf,
    const int32_t bufferId,
    const int64_t byteLength,
    const int64_t byteStride) {
  size_t bufferViewId = gltf.bufferViews.size();
  BufferView& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = bufferId;
  bufferView.byteLength = byteLength;
  bufferView.byteOffset = 0;
  bufferView.byteStride = byteStride;
  bufferView.target = BufferView::Target::ARRAY_BUFFER;

  return static_cast<int32_t>(bufferViewId);
}

int32_t createAccessorInGltf(
    Model& gltf,
    const int32_t bufferViewId,
    const int32_t componentType,
    const int64_t count,
    const std::string type) {
  size_t accessorId = gltf.accessors.size();
  Accessor& accessor = gltf.accessors.emplace_back();
  accessor.bufferView = bufferViewId;
  accessor.byteOffset = 0;
  accessor.componentType = componentType;
  accessor.count = count;
  accessor.type = type;

  return static_cast<int32_t>(accessorId);
}

void addPositionsToGltf(PntsContent& parsedContent, Model& gltf) {
  const int64_t count = static_cast<int64_t>(parsedContent.pointsLength);
  const int64_t byteStride = static_cast<int64_t>(sizeof(glm ::vec3));
  const int64_t byteLength = static_cast<int64_t>(byteStride * count);
  int32_t bufferId = createBufferInGltf(gltf, parsedContent.position.data);
  int32_t bufferViewId =
      createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);
  int32_t accessorId = createAccessorInGltf(
      gltf,
      bufferViewId,
      Accessor::ComponentType::FLOAT,
      count,
      Accessor::Type::VEC3);

  Accessor& accessor = gltf.accessors[static_cast<uint32_t>(accessorId)];
  accessor.min = {
      parsedContent.positionMin.x,
      parsedContent.positionMin.y,
      parsedContent.positionMin.z,
  };
  accessor.max = {
      parsedContent.positionMax.x,
      parsedContent.positionMax.y,
      parsedContent.positionMax.z,
  };

  MeshPrimitive& primitive = gltf.meshes[0].primitives[0];
  primitive.attributes.emplace("POSITION", accessorId);
}

void addColorsToGltf(PntsContent& parsedContent, Model& gltf) {
  PntsSemantic& color = parsedContent.color.value();

  const int64_t count = static_cast<int64_t>(parsedContent.pointsLength);
  int64_t byteStride = 0;
  int32_t componentType = 0;
  std::string type;
  bool isTranslucent = false;
  bool isNormalized = false;

  if (parsedContent.colorType == PntsColorType::RGBA) {
    byteStride = static_cast<int64_t>(sizeof(glm::u8vec4));
    componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    type = Accessor::Type::VEC4;
    isTranslucent = true;
    isNormalized = true;
  } else if (parsedContent.colorType == PntsColorType::RGB) {
    byteStride = static_cast<int64_t>(sizeof(glm::u8vec3));
    componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    isNormalized = true;
    type = Accessor::Type::VEC3;
  } else if (parsedContent.colorType == PntsColorType::RGB565) {
    byteStride = static_cast<int64_t>(sizeof(glm::vec3));
    componentType = Accessor::ComponentType::FLOAT;
    type = Accessor::Type::VEC3;
  }

  const int64_t byteLength = static_cast<int64_t>(byteStride * count);
  int32_t bufferId = createBufferInGltf(gltf, color.data);
  int32_t bufferViewId =
      createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);
  int32_t accessorId =
      createAccessorInGltf(gltf, bufferViewId, componentType, count, type);

  Accessor& accessor = gltf.accessors[static_cast<uint32_t>(accessorId)];
  accessor.normalized = isNormalized;

  MeshPrimitive& primitive = gltf.meshes[0].primitives[0];
  primitive.attributes.emplace("COLOR_0", accessorId);

  if (isTranslucent) {
    Material& material =
        gltf.materials[static_cast<uint32_t>(primitive.material)];
    material.alphaMode = Material::AlphaMode::BLEND;
  }
}

void addNormalsToGltf(PntsContent& parsedContent, Model& gltf) {
  PntsSemantic& normal = parsedContent.normal.value();

  const int64_t count = static_cast<int64_t>(parsedContent.pointsLength);
  const int64_t byteStride = static_cast<int64_t>(sizeof(glm ::vec3));
  const int64_t byteLength = static_cast<int64_t>(byteStride * count);

  int32_t bufferId = createBufferInGltf(gltf, normal.data);
  int32_t bufferViewId =
      createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);
  int32_t accessorId = createAccessorInGltf(
      gltf,
      bufferViewId,
      Accessor::ComponentType::FLOAT,
      count,
      Accessor::Type::VEC3);

  MeshPrimitive& primitive = gltf.meshes[0].primitives[0];
  primitive.attributes.emplace("NORMAL", accessorId);
}

void addBatchIdsToGltf(PntsContent& parsedContent, CesiumGltf::Model& gltf) {
  PntsSemantic& batchId = parsedContent.batchId.value();

  const int64_t count = static_cast<int64_t>(parsedContent.pointsLength);
  int32_t componentType = Accessor::ComponentType::UNSIGNED_SHORT;
  if (parsedContent.batchIdComponentType) {
    componentType = parsedContent.batchIdComponentType.value();
  }
  const int64_t byteStride =
      Accessor::computeByteSizeOfComponent(componentType);
  const int64_t byteLength = static_cast<int64_t>(byteStride * count);

  int32_t bufferId = createBufferInGltf(gltf, batchId.data);
  int32_t bufferViewId =
      createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);
  int32_t accessorId = createAccessorInGltf(
      gltf,
      bufferViewId,
      componentType,
      count,
      Accessor::Type::SCALAR);

  MeshPrimitive& primitive = gltf.meshes[0].primitives[0];
  // This will be renamed by BatchTableToGltfFeatureMetadata.
  primitive.attributes.emplace("_BATCHID", accessorId);
}

void createGltfFromParsedContent(
    PntsContent& parsedContent,
    GltfConverterResult& result) {
  result.model = std::make_optional<CesiumGltf::Model>();
  Model& gltf = result.model.value();

  // Create a single node with a single mesh, with a single primitive.
  Node& node = gltf.nodes.emplace_back();
  std::memcpy(
      node.matrix.data(),
      &CesiumGeometry::AxisTransforms::Z_UP_TO_Y_UP,
      sizeof(glm::dmat4));

  size_t meshId = gltf.meshes.size();
  Mesh& mesh = gltf.meshes.emplace_back();
  node.mesh = static_cast<int32_t>(meshId);

  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  primitive.mode = MeshPrimitive::Mode::POINTS;

  size_t materialId = gltf.materials.size();
  Material& material = gltf.materials.emplace_back();
  material.pbrMetallicRoughness =
      std::make_optional<CesiumGltf::MaterialPBRMetallicRoughness>();
  // These values are borrowed from CesiumJS.
  material.pbrMetallicRoughness.value().metallicFactor = 0;
  material.pbrMetallicRoughness.value().roughnessFactor = 0.9;

  primitive.material = static_cast<int32_t>(materialId);

  addPositionsToGltf(parsedContent, gltf);

  if (parsedContent.color) {
    addColorsToGltf(parsedContent, gltf);
  } else if (parsedContent.constantRgba) {
    // Map RGBA from [0, 255] to [0, 1]
    glm::vec4 materialColor(parsedContent.constantRgba.value());
    materialColor /= 255.0f;

    material.pbrMetallicRoughness.value().baseColorFactor =
        {materialColor.x, materialColor.y, materialColor.z, materialColor.w};
    material.alphaMode = CesiumGltf::Material::AlphaMode::BLEND;
  }

  if (parsedContent.normal) {
    addNormalsToGltf(parsedContent, gltf);
  } else {
    // Points without normals should be rendered without lighting, which we
    // can indicate with the KHR_materials_unlit extension.
    material.addExtension<CesiumGltf::ExtensionKhrMaterialsUnlit>();
  }

  if (parsedContent.batchId) {
    addBatchIdsToGltf(parsedContent, gltf);
  }

  if (parsedContent.rtcCenter) {
    // Add the RTC_CENTER value to the glTF as a CESIUM_RTC extension.
    // This matches what B3dmToGltfConverter does. In the future,
    // this can be added instead to the translation component of
    // the root node, as suggested in the 3D Tiles migration guide.
    auto& cesiumRTC =
        result.model->addExtension<CesiumGltf::ExtensionCesiumRTC>();
    glm::dvec3 rtcCenter = parsedContent.rtcCenter.value();
    cesiumRTC.center = {rtcCenter.x, rtcCenter.y, rtcCenter.z};
  }
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
    const int64_t batchTableStart = headerLength +
                                    header.featureTableJsonByteLength +
                                    header.featureTableBinaryByteLength;

    rapidjson::Document batchTableJson;
    if (header.batchTableJsonByteLength > 0) {
      const gsl::span<const std::byte> batchTableJsonData = pntsBinary.subspan(
          static_cast<size_t>(batchTableStart),
          header.batchTableJsonByteLength);

      batchTableJson = parseBatchTableJson(batchTableJsonData, parsedContent);
      if (parsedContent.errors) {
        result.errors.merge(parsedContent.errors);
        return;
      }
    }

    const gsl::span<const std::byte> featureTableBinaryData =
        pntsBinary.subspan(
            static_cast<size_t>(
                headerLength + header.featureTableJsonByteLength),
            header.featureTableBinaryByteLength);

    parseFeatureTableBinary(featureTableBinaryData, parsedContent);
    createGltfFromParsedContent(parsedContent, result);

    if (header.batchTableJsonByteLength > 0) {
      gsl::span<const std::byte> batchTableBinaryData;
      // check if data was manipulated earlier
      if (header.batchTableBinaryByteLength > 0) {
        batchTableBinaryData = pntsBinary.subspan(
            static_cast<size_t>(
                batchTableStart + header.batchTableJsonByteLength),
            header.batchTableBinaryByteLength);
      }

      result.errors.merge(BatchTableToGltfFeatureMetadata::convertFromPnts(
          featureTableJson,
          batchTableJson,
          batchTableBinaryData,
          result.model.value()));
    }
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
