#include "BatchTableToGltfStructuralMetadata.h"
#include "MetadataProperty.h"

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <Cesium3DTilesContent/PntsToGltfConverter.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/ExtensionKhrMaterialsUnlit.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/AttributeCompression.h>

#include <draco/core/data_buffer.h>
#include <draco/core/draco_types.h>
#include <draco/core/status_or.h>
#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/ext/vector_uint4_sized.hpp>
#include <rapidjson/rapidjson.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

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

#include <rapidjson/document.h>

#include <limits>

using namespace CesiumGltf;
using namespace CesiumUtility;

namespace Cesium3DTilesContent {
namespace {
struct PntsHeader {
  unsigned char magic[4] = {0, 0, 0, 0};
  uint32_t version = 0;
  uint32_t byteLength = 0;
  uint32_t featureTableJsonByteLength = 0;
  uint32_t featureTableBinaryByteLength = 0;
  uint32_t batchTableJsonByteLength = 0;
  uint32_t batchTableBinaryByteLength = 0;
};

void parsePntsHeader(
    const std::span<const std::byte>& pntsBinary,
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

struct DracoMetadataSemantic {
  int32_t dracoId;
  MetadataProperty::ComponentType componentType;
  MetadataProperty::Type type;
};

enum PntsColorType { CONSTANT, RGBA, RGB, RGB565 };

// Point cloud colors are stored in sRGB space, so they need to be converted to
// linear RGB for the glTF.
// This function assumes the sRGB values are normalized from [0, 255] to [0, 1]
template <typename TColor> TColor srgbToLinear(const TColor srgb) {
  static_assert(
      std::is_same_v<TColor, glm::vec3> || std::is_same_v<TColor, glm::vec4>);

  glm::vec3 srgbInput = glm::vec3(srgb);
  glm::vec3 linearOutput = glm::pow(srgbInput, glm::vec3(2.2f));

  if constexpr (std::is_same_v<TColor, glm::vec4>) {
    return glm::vec4(linearOutput, srgb.w);
  } else if constexpr (std::is_same_v<TColor, glm::vec3>) {
    return linearOutput;
  }
}

struct PntsContent {
  uint32_t pointsLength = 0;
  std::optional<glm::dvec3> rtcCenter;
  std::optional<glm::dvec3> quantizedVolumeOffset;
  std::optional<glm::dvec3> quantizedVolumeScale;
  std::optional<glm::u8vec4> constantRgba;
  std::optional<uint32_t> batchLength;

  PntsSemantic position;
  bool positionQuantized = false;
  // required by glTF spec
  glm::vec3 positionMin = glm::vec3(std::numeric_limits<float>::max());
  glm::vec3 positionMax = glm::vec3(std::numeric_limits<float>::lowest());

  std::optional<PntsSemantic> color;
  PntsColorType colorType = PntsColorType::CONSTANT;

  std::optional<PntsSemantic> normal;
  bool normalOctEncoded = false;

  std::optional<PntsSemantic> batchId;
  std::optional<MetadataProperty::ComponentType> batchIdComponentType;

  std::optional<uint32_t> dracoByteOffset;
  std::optional<uint32_t> dracoByteLength;

  std::map<std::string, DracoMetadataSemantic> dracoMetadataSemantics;
  std::vector<std::byte> dracoBatchTableBinary;

  ErrorList errors;
  bool dracoMetadataHasErrors = false;
};

template <typename TValidate>
bool validateJsonArrayValues(
    const rapidjson::Value& arrayValue,
    uint32_t expectedLength,
    TValidate validate) {
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
          "Error parsing PNTS feature table, POSITION does not have "
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
          "no valid QUANTIZED_VOLUME_OFFSET was found.");
      return;
    }

    if (quantizedVolumeScaleIt == featureTableJson.MemberEnd() ||
        !validateJsonArrayValues(quantizedVolumeScaleIt->value, 3, isNumber)) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, POSITION_QUANTIZED is used but "
          "no valid QUANTIZED_VOLUME_SCALE was found.");
      return;
    }

    const auto byteOffsetIt =
        positionQuantizedIt->value.FindMember("byteOffset");
    if (byteOffsetIt == positionQuantizedIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceError(
          "Error parsing PNTS feature table, POSITION_QUANTIZED does not have "
          "valid byteOffset.");
      return;
    }

    parsedContent.positionQuantized = true;
    parsedContent.position.byteOffset = byteOffsetIt->value.GetUint();

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
      "Error parsing PNTS feature table, one of POSITION or POSITION_QUANTIZED "
      "must be defined.");

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
        "Error parsing PNTS feature table, RGBA does not have valid "
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
        "Error parsing PNTS feature table, RGB does not have valid byteOffset. "
        "Skip parsing RGB colors.");
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
        "Error parsing PNTS feature table, RGB565 does not have valid "
        "byteOffset. Skip parsing RGB565 colors.");
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
        "Error parsing PNTS feature table, NORMAL does not have valid "
        "byteOffset. Skip parsing normals.");
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
        "Error parsing PNTS feature table, NORMAL_OCT16P does not have valid "
        "byteOffset. Skip parsing oct-encoded normals.");
  }
}

void parseBatchIdsFromFeatureTableJson(
    const rapidjson::Document& featureTableJson,
    PntsContent& parsedContent) {
  const auto batchIdIt = featureTableJson.FindMember("BATCH_ID");
  if (batchIdIt == featureTableJson.MemberEnd() ||
      !batchIdIt->value.IsObject()) {
    return;
  }

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
    const std::string& componentTypeString = componentTypeIt->value.GetString();
    if (MetadataProperty::stringToMetadataComponentType.find(
            componentTypeString) ==
        MetadataProperty::stringToMetadataComponentType.end()) {
      parsedContent.errors.emplaceWarning(
          "Error parsing PNTS feature table, BATCH_ID does not have "
          "valid componentType. Skip parsing batch IDs.");
      return;
    }

    MetadataProperty::ComponentType componentType =
        MetadataProperty::stringToMetadataComponentType.at(componentTypeString);
    if (componentType != MetadataProperty::ComponentType::UNSIGNED_BYTE &&
        componentType != MetadataProperty::ComponentType::UNSIGNED_SHORT &&
        componentType != MetadataProperty::ComponentType::UNSIGNED_INT) {
      parsedContent.errors.emplaceWarning(
          "Error parsing PNTS feature table, BATCH_ID componentType is defined "
          "but is not UNSIGNED_BYTE, UNSIGNED_SHORT, or UNSIGNED_INT. Skip "
          "parsing batch IDs.");
      return;
    }
    parsedContent.batchIdComponentType = componentType;
  } else {
    parsedContent.batchIdComponentType =
        MetadataProperty::ComponentType::UNSIGNED_SHORT;
  }

  const auto batchLengthIt = featureTableJson.FindMember("BATCH_LENGTH");
  if (batchLengthIt != featureTableJson.MemberEnd() &&
      batchLengthIt->value.IsUint()) {
    parsedContent.batchLength = batchLengthIt->value.GetUint();
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
    parsedContent.rtcCenter = glm::vec3(
        rtcValue[0].GetDouble(),
        rtcValue[1].GetDouble(),
        rtcValue[2].GetDouble());
  }
}

void parseDracoExtensionFromFeatureTableJson(
    const rapidjson::Value& dracoExtension,
    PntsContent& parsedContent) {
  const auto propertiesIt = dracoExtension.FindMember("properties");
  if (propertiesIt == dracoExtension.MemberEnd() ||
      !propertiesIt->value.IsObject()) {
    parsedContent.errors.emplaceError(
        "Error parsing 3DTILES_draco_compression extension, "
        "no valid properties object found.");
    return;
  }

  const auto byteOffsetIt = dracoExtension.FindMember("byteOffset");
  if (byteOffsetIt == dracoExtension.MemberEnd() ||
      !byteOffsetIt->value.IsUint64()) {
    parsedContent.errors.emplaceError(
        "Error parsing 3DTILES_draco_compression extension, "
        "no valid byteOffset found.");
    return;
  }

  const auto byteLengthIt = dracoExtension.FindMember("byteLength");
  if (byteLengthIt == dracoExtension.MemberEnd() ||
      !byteLengthIt->value.IsUint64()) {
    parsedContent.errors.emplaceError(
        "Error parsing 3DTILES_draco_compression extension, "
        "no valid byteLength found.");
    return;
  }

  parsedContent.dracoByteOffset = byteOffsetIt->value.GetUint();
  parsedContent.dracoByteLength = byteLengthIt->value.GetUint();

  const rapidjson::Value& dracoProperties = propertiesIt->value;
  auto positionDracoIdIt = dracoProperties.FindMember("POSITION");
  if (positionDracoIdIt != dracoProperties.MemberEnd() &&
      positionDracoIdIt->value.IsInt()) {
    parsedContent.position.dracoId = positionDracoIdIt->value.GetInt();
  }

  if (parsedContent.color) {
    if (parsedContent.colorType == PntsColorType::RGBA) {
      auto rgbaDracoIdIt = dracoProperties.FindMember("RGBA");
      if (rgbaDracoIdIt != dracoProperties.MemberEnd() &&
          rgbaDracoIdIt->value.IsInt()) {
        parsedContent.color.value().dracoId = rgbaDracoIdIt->value.GetInt();
      }
    } else if (parsedContent.colorType == PntsColorType::RGB) {
      auto rgbDracoIdIt = dracoProperties.FindMember("RGB");
      if (rgbDracoIdIt != dracoProperties.MemberEnd() &&
          rgbDracoIdIt->value.IsInt()) {
        parsedContent.color.value().dracoId = rgbDracoIdIt->value.GetInt();
      }
    }
  }

  if (parsedContent.normal) {
    auto normalDracoIdIt = dracoProperties.FindMember("NORMAL");
    if (normalDracoIdIt != dracoProperties.MemberEnd() &&
        normalDracoIdIt->value.IsInt()) {
      parsedContent.normal.value().dracoId = normalDracoIdIt->value.GetInt();
    }
  }

  if (parsedContent.batchId) {
    auto batchIdDracoIdIt = dracoProperties.FindMember("BATCH_ID");
    if (batchIdDracoIdIt != dracoProperties.MemberEnd() &&
        batchIdDracoIdIt->value.IsInt()) {
      parsedContent.batchId.value().dracoId = batchIdDracoIdIt->value.GetInt();
    }
  }
}

rapidjson::Document parseFeatureTableJson(
    const std::span<const std::byte>& featureTableJsonData,
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
        "Error parsing PNTS feature table, no valid POINTS_LENGTH was found.");
    return document;
  }
  parsedContent.pointsLength = pointsLengthIt->value.GetUint();

  if (parsedContent.pointsLength == 0) {
    // This *should* be disallowed by the spec, but it currently isn't.
    // In the future, this can be converted to an error.
    parsedContent.errors.emplaceWarning("The PNTS has a POINTS_LENGTH of zero. "
                                        "Skip parsing the PNTS feature table.");
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
      const rapidjson::Value& dracoExtension = dracoExtensionIt->value;
      parseDracoExtensionFromFeatureTableJson(dracoExtension, parsedContent);
    }
  }
  return document;
}

void parseDracoExtensionFromBatchTableJson(
    const rapidjson::Document& batchTableJson,
    PntsContent& parsedContent) {
  const auto extensionsIt = batchTableJson.FindMember("extensions");
  if (extensionsIt == batchTableJson.MemberEnd() ||
      !extensionsIt->value.IsObject()) {
    return;
  }

  const auto dracoExtensionIt =
      extensionsIt->value.FindMember("3DTILES_draco_point_compression");
  if (dracoExtensionIt == extensionsIt->value.MemberEnd() ||
      !dracoExtensionIt->value.IsObject()) {
    return;
  }

  if (parsedContent.batchLength) {
    parsedContent.errors.emplaceWarning(
        "Error parsing batch table, 3DTILES_draco_point_compression is present "
        "but BATCH_LENGTH is defined. Only per-point properties can be "
        "compressed by Draco.");
    parsedContent.dracoMetadataHasErrors = true;
    return;
  }

  const auto propertiesIt = dracoExtensionIt->value.FindMember("properties");
  if (propertiesIt == dracoExtensionIt->value.MemberEnd() ||
      !propertiesIt->value.IsObject()) {
    parsedContent.errors.emplaceWarning(
        "Error parsing 3DTILES_draco_point_compression extension, no "
        "properties object was found.");
    parsedContent.dracoMetadataHasErrors = true;
    return;
  }

  const rapidjson::Value& properties = propertiesIt->value.GetObject();
  for (auto dracoPropertyIt = properties.MemberBegin();
       dracoPropertyIt != properties.MemberEnd();
       ++dracoPropertyIt) {
    const std::string name = dracoPropertyIt->name.GetString();

    // If there are issues with the extension, skip parsing metadata altogether.
    // Otherwise, BatchTableToGltfStructuralMetadata will still try to parse the
    // invalid Draco-compressed properties.
    auto batchTablePropertyIt = batchTableJson.FindMember(name.c_str());
    if (batchTablePropertyIt == batchTableJson.MemberEnd() ||
        !batchTablePropertyIt->value.IsObject()) {
      parsedContent.errors.emplaceWarning(fmt::format(
          "The metadata property {} is in the 3DTILES_draco_point_compression "
          "extension but not in the batch table itself.",
          name));
      continue;
    }

    if (!dracoPropertyIt->value.IsInt()) {
      parsedContent.errors.emplaceWarning(fmt::format(
          "Error parsing 3DTILES_draco_compression extension, the metadata "
          "property {} does not have valid draco ID. Skip parsing metadata.",
          name));
      parsedContent.dracoMetadataHasErrors = true;
      return;
    }

    // If the batch table binary property is invalid, it will also be ignored by
    // BatchTableToGltfStructuralMetadata, so it's fine to continue parsing the
    // other properties.
    const rapidjson::Value& batchTableProperty = batchTablePropertyIt->value;
    auto byteOffsetIt = batchTableProperty.FindMember("byteOffset");
    if (byteOffsetIt == batchTableProperty.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      parsedContent.errors.emplaceWarning(fmt::format(
          "Skip decoding Draco-compressed property {}. The binary property "
          "doesn't have a valid byteOffset.",
          name));
      continue;
    }

    std::string componentType;
    auto componentTypeIt = batchTableProperty.FindMember("componentType");
    if (componentTypeIt != batchTableProperty.MemberEnd() &&
        componentTypeIt->value.IsString()) {
      componentType = componentTypeIt->value.GetString();
    }
    if (MetadataProperty::stringToMetadataComponentType.find(componentType) ==
        MetadataProperty::stringToMetadataComponentType.end()) {
      parsedContent.errors.emplaceWarning(fmt::format(
          "Skip decoding Draco-compressed property {}. The binary property "
          "doesn't have a valid componentType.",
          name));
      continue;
    }

    std::string type;
    auto typeIt = batchTableProperty.FindMember("type");
    if (typeIt != batchTableProperty.MemberEnd() && typeIt->value.IsString()) {
      type = typeIt->value.GetString();
    }
    if (MetadataProperty::stringToMetadataType.find(type) ==
        MetadataProperty::stringToMetadataType.end()) {
      parsedContent.errors.emplaceWarning(fmt::format(
          "Skip decoding Draco-compressed property {}. The binary property "
          "doesn't have a valid type.",
          name));
      continue;
    }

    DracoMetadataSemantic semantic;
    semantic.dracoId = dracoPropertyIt->value.GetInt();
    semantic.componentType =
        MetadataProperty::stringToMetadataComponentType.at(componentType);
    semantic.type = MetadataProperty::stringToMetadataType.at(type);

    parsedContent.dracoMetadataSemantics.insert({name, semantic});
  }
}

rapidjson::Document parseBatchTableJson(
    const std::span<const std::byte>& batchTableJsonData,
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

  parseDracoExtensionFromBatchTableJson(document, parsedContent);

  return document;
}

bool validateDracoAttribute(
    const draco::PointAttribute* const pAttribute,
    const draco::DataType expectedDataType,
    const int32_t expectedNumComponents) {
  return pAttribute && pAttribute->data_type() == expectedDataType &&
         pAttribute->num_components() == expectedNumComponents;
}

std::optional<MetadataProperty::ComponentType>
getComponentTypeFromDracoDataType(const draco::DataType dataType) {
  switch (dataType) {
  case draco::DT_INT8:
    return MetadataProperty::ComponentType::BYTE;
  case draco::DT_UINT8:
    return MetadataProperty::ComponentType::UNSIGNED_BYTE;
  case draco::DT_INT16:
    return MetadataProperty::ComponentType::SHORT;
  case draco::DT_UINT16:
    return MetadataProperty::ComponentType::UNSIGNED_SHORT;
  case draco::DT_INT32:
    return MetadataProperty::ComponentType::INT;
  case draco::DT_UINT32:
    return MetadataProperty::ComponentType::UNSIGNED_INT;
  case draco::DT_FLOAT32:
    return MetadataProperty::ComponentType::FLOAT;
  case draco::DT_FLOAT64:
    return MetadataProperty::ComponentType::DOUBLE;
  default:
    return std::nullopt;
  }
}

bool validateDracoMetadataAttribute(
    const draco::PointAttribute* const pAttribute,
    const DracoMetadataSemantic semantic) {
  if (!pAttribute) {
    return false;
  }

  auto componentType =
      getComponentTypeFromDracoDataType(pAttribute->data_type());
  if (!componentType || componentType.value() != semantic.componentType) {
    return false;
  }

  auto type = MetadataProperty::getTypeFromNumberOfComponents(
      static_cast<int8_t>(pAttribute->num_components()));
  return type && type.value() == semantic.type;
}

template <typename T>
void getDracoData(
    const draco::PointAttribute* pAttribute,
    std::vector<std::byte>& data,
    const uint32_t pointsLength) {
  const size_t dataElementSize = sizeof(T);
  const size_t databufferByteLength = pointsLength * dataElementSize;
  data.resize(databufferByteLength);

  draco::DataBuffer* decodedBuffer = pAttribute->buffer();
  int64_t decodedByteOffset = pAttribute->byte_offset();
  int64_t decodedByteStride = pAttribute->byte_stride();

  const uint8_t* pSource = decodedBuffer->data() + decodedByteOffset;
  if (dataElementSize != static_cast<size_t>(decodedByteStride)) {
    std::span<T> outData(reinterpret_cast<T*>(data.data()), pointsLength);
    for (uint32_t i = 0; i < pointsLength; ++i) {
      outData[i] = *reinterpret_cast<const T*>(pSource + decodedByteStride * i);
    }
  } else {
    std::memcpy(
        data.data(),
        decodedBuffer->data() + decodedByteOffset,
        databufferByteLength);
  }
}

void decodeDracoMetadata(
    const std::unique_ptr<draco::PointCloud>& pPointCloud,
    rapidjson::Document& batchTableJson,
    PntsContent& parsedContent) {
  const uint64_t pointsLength = parsedContent.pointsLength;
  std::vector<std::byte>& data = parsedContent.dracoBatchTableBinary;

  const auto& dracoMetadataSemantics = parsedContent.dracoMetadataSemantics;
  for (const auto& dracoMetadataSemantic : dracoMetadataSemantics) {
    const DracoMetadataSemantic& dracoSemantic = dracoMetadataSemantic.second;
    draco::PointAttribute* pAttribute =
        pPointCloud->attribute(dracoSemantic.dracoId);
    if (!validateDracoMetadataAttribute(pAttribute, dracoSemantic)) {
      parsedContent.errors.emplaceWarning(fmt::format(
          "Error decoding {} property in the 3DTILES_draco_compression "
          "extension. Skip parsing metadata.",
          dracoMetadataSemantic.first));
      parsedContent.dracoMetadataHasErrors = true;
      return;
    }

    const size_t byteOffset = data.size();

    // These do not test for validity since the batch table and extension
    // were validated in parseDracoExtensionFromBatchTableJson.
    auto batchTableSemanticIt =
        batchTableJson.FindMember(dracoMetadataSemantic.first.c_str());
    rapidjson::Value& batchTableSemantic =
        batchTableSemanticIt->value.GetObject();
    auto byteOffsetIt = batchTableSemantic.FindMember("byteOffset");
    byteOffsetIt->value.SetUint(static_cast<uint32_t>(byteOffset));

    const size_t metadataByteStride =
        MetadataProperty::getSizeOfComponentType(dracoSemantic.componentType) *
        static_cast<size_t>(pAttribute->num_components());

    data.resize(byteOffset + pointsLength * metadataByteStride);
    draco::DataBuffer* decodedBuffer = pAttribute->buffer();
    int64_t decodedByteOffset = pAttribute->byte_offset();
    int64_t decodedByteStride = pAttribute->byte_stride();

    if (metadataByteStride != static_cast<size_t>(decodedByteStride)) {
      for (uint32_t i = 0; i < pointsLength; ++i) {
        std::memcpy(
            data.data() + byteOffset + i * metadataByteStride,
            decodedBuffer->data() + decodedByteOffset + decodedByteStride * i,
            metadataByteStride);
      }
    } else {
      std::memcpy(
          data.data() + byteOffset,
          decodedBuffer->data() + decodedByteOffset,
          metadataByteStride * pointsLength);
    }
  }
}

void decodeDraco(
    const std::span<const std::byte>& featureTableBinaryData,
    rapidjson::Document& batchTableJson,
    const std::span<const std::byte>& batchTableBinaryData,
    PntsContent& parsedContent) {
  if (!parsedContent.dracoByteOffset || !parsedContent.dracoByteLength) {
    return;
  }

  draco::Decoder decoder;
  draco::DecoderBuffer buffer;
  buffer.Init(
      reinterpret_cast<const char*>(featureTableBinaryData.data()) +
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

    std::span<glm::vec3> outPositions(
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
      draco::PointAttribute* pColorAttribute =
          pPointCloud->attribute(color.dracoId.value());
      std::vector<std::byte>& colorData = parsedContent.color->data;
      if (parsedContent.colorType == PntsColorType::RGBA &&
          validateDracoAttribute(pColorAttribute, draco::DT_UINT8, 4)) {
        colorData.resize(pointsLength * sizeof(glm::vec4));

        std::span<glm::vec4> outColors(
            reinterpret_cast<glm::vec4*>(colorData.data()),
            pointsLength);

        draco::DataBuffer* decodedBuffer = pColorAttribute->buffer();
        int64_t decodedByteOffset = pColorAttribute->byte_offset();
        int64_t decodedByteStride = pColorAttribute->byte_stride();

        for (uint32_t i = 0; i < pointsLength; ++i) {
          const glm::u8vec4 rgbaColor = *reinterpret_cast<const glm::u8vec4*>(
              decodedBuffer->data() + decodedByteOffset +
              decodedByteStride * i);
          outColors[i] = srgbToLinear(glm::vec4(rgbaColor) / 255.0f);
        }
      } else if (
          parsedContent.colorType == PntsColorType::RGB &&
          validateDracoAttribute(pColorAttribute, draco::DT_UINT8, 3)) {
        colorData.resize(pointsLength * sizeof(glm::vec3));

        std::span<glm::vec3> outColors(
            reinterpret_cast<glm::vec3*>(colorData.data()),
            pointsLength);

        draco::DataBuffer* decodedBuffer = pColorAttribute->buffer();
        int64_t decodedByteOffset = pColorAttribute->byte_offset();
        int64_t decodedByteStride = pColorAttribute->byte_stride();

        for (uint32_t i = 0; i < pointsLength; ++i) {
          const glm::u8vec3 rgbColor = *reinterpret_cast<const glm::u8vec3*>(
              decodedBuffer->data() + decodedByteOffset +
              decodedByteStride * i);
          outColors[i] = srgbToLinear(glm::vec3(rgbColor) / 255.0f);
        }
      } else {
        parsedContent.errors.emplaceWarning(
            "Error parsing decoded Draco point cloud, invalid color attribute. "
            "Skip parsing colors.");
        parsedContent.color = std::nullopt;
        parsedContent.colorType = PntsColorType::CONSTANT;
      }
    }
  }

  if (parsedContent.normal) {
    PntsSemantic& normal = parsedContent.normal.value();
    if (normal.dracoId) {
      draco::PointAttribute* pNormalAttribute =
          pPointCloud->attribute(normal.dracoId.value());
      if (validateDracoAttribute(pNormalAttribute, draco::DT_FLOAT32, 3)) {
        getDracoData<glm::vec3>(pNormalAttribute, normal.data, pointsLength);
      } else {
        parsedContent.errors.emplaceWarning("Error parsing decoded Draco point "
                                            "cloud, invalid normal attribute. "
                                            "Skip parsing normals.");
        parsedContent.normal = std::nullopt;
      }
    }
  }

  if (parsedContent.batchId) {
    PntsSemantic& batchId = parsedContent.batchId.value();
    if (batchId.dracoId) {
      draco::PointAttribute* pBatchIdAttribute =
          pPointCloud->attribute(batchId.dracoId.value());

      int32_t componentType = 0;
      if (parsedContent.batchIdComponentType) {
        componentType = parsedContent.batchIdComponentType.value();
      }

      if (componentType == MetadataProperty::ComponentType::UNSIGNED_BYTE &&
          validateDracoAttribute(pBatchIdAttribute, draco::DT_UINT8, 1)) {
        getDracoData<uint8_t>(pBatchIdAttribute, batchId.data, pointsLength);
      } else if (
          componentType == MetadataProperty::ComponentType::UNSIGNED_INT &&
          validateDracoAttribute(pBatchIdAttribute, draco::DT_UINT32, 1)) {
        getDracoData<uint32_t>(pBatchIdAttribute, batchId.data, pointsLength);
      } else if (
          (componentType == 0 ||
           componentType == MetadataProperty::ComponentType::UNSIGNED_SHORT) &&
          validateDracoAttribute(pBatchIdAttribute, draco::DT_UINT16, 1)) {
        getDracoData<uint16_t>(pBatchIdAttribute, batchId.data, pointsLength);
      } else {
        parsedContent.errors.emplaceWarning(
            "Error parsing decoded Draco point cloud, invalid batch ID "
            "attribute. "
            "Skip parsing batch IDs.");
        parsedContent.batchId = std::nullopt;
      }
    }
  }

  if (batchTableJson.HasParseError() || parsedContent.dracoMetadataHasErrors) {
    return;
  }

  // Not all metadata attributes are necessarily compressed. Copy the binary of
  // the uncompressed attributes first, before appending the decoded data.
  size_t batchTableBinaryByteLength = batchTableBinaryData.size();
  if (batchTableBinaryByteLength > 0) {
    parsedContent.dracoBatchTableBinary.resize(batchTableBinaryByteLength);
    std::memcpy(
        parsedContent.dracoBatchTableBinary.data(),
        batchTableBinaryData.data(),
        batchTableBinaryByteLength);
  }

  decodeDracoMetadata(pPointCloud, batchTableJson, parsedContent);
}

void parsePositionsFromFeatureTableBinary(
    const std::span<const std::byte>& featureTableBinaryData,
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

  std::span<glm::vec3> outPositions(
      reinterpret_cast<glm::vec3*>(positionData.data()),
      pointsLength);

  if (parsedContent.positionQuantized && parsedContent.quantizedVolumeScale &&
      parsedContent.quantizedVolumeOffset) {
    // PERFORMANCE_IDEA: In the future, it might be more performant to detect
    // if the recipient engine can handle dequantization itself, and if so, use
    // the KHR_mesh_quantization extension to avoid dequantizing here.
    const std::span<const glm::u16vec3> quantizedPositions(
        reinterpret_cast<const glm::u16vec3*>(
            featureTableBinaryData.data() + parsedContent.position.byteOffset),
        pointsLength);

    const glm::vec3 quantizedVolumeScale(
        parsedContent.quantizedVolumeScale.value());
    const glm::vec3 quantizedVolumeOffset(
        parsedContent.quantizedVolumeOffset.value());

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
  } else if (parsedContent.positionQuantized) {
    parsedContent.errors.emplaceError(
        "Missing quantizedVolumeScale or quantizedVolumeOffset in parsed "
        "content");
  } else {
    // The position accessor min / max is required by the glTF spec, so
    // use a for loop instead of std::memcpy.
    const std::span<const glm::vec3> positions(
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
    const std::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  CESIUM_ASSERT(parsedContent.color.has_value());
  PntsSemantic& color = parsedContent.color.value();
  std::vector<std::byte>& colorData = color.data;
  if (colorData.size() > 0) {
    // If data isn't empty, it must have been decoded from Draco.
    return;
  }

  const uint32_t pointsLength = parsedContent.pointsLength;
  const size_t colorsByteStride = parsedContent.colorType == PntsColorType::RGBA
                                      ? sizeof(glm::vec4)
                                      : sizeof(glm::vec3);
  const size_t colorsByteLength = pointsLength * colorsByteStride;
  colorData.resize(colorsByteLength);

  if (parsedContent.colorType == PntsColorType::RGBA) {
    const std::span<const glm::u8vec4> rgbaColors(
        reinterpret_cast<const glm::u8vec4*>(
            featureTableBinaryData.data() + color.byteOffset),
        pointsLength);
    std::span<glm::vec4> outColors(
        reinterpret_cast<glm::vec4*>(colorData.data()),
        pointsLength);

    for (size_t i = 0; i < pointsLength; i++) {
      glm::vec4 normalizedColor = glm::vec4(rgbaColors[i]) / 255.0f;
      outColors[i] = srgbToLinear(normalizedColor);
    }
  } else if (parsedContent.colorType == PntsColorType::RGB) {
    const std::span<const glm::u8vec3> rgbColors(
        reinterpret_cast<const glm::u8vec3*>(
            featureTableBinaryData.data() + color.byteOffset),
        pointsLength);
    std::span<glm::vec3> outColors(
        reinterpret_cast<glm::vec3*>(colorData.data()),
        pointsLength);

    for (size_t i = 0; i < pointsLength; i++) {
      glm::vec3 normalizedColor = glm::vec3(rgbColors[i]) / 255.0f;
      outColors[i] = srgbToLinear(normalizedColor);
    }
  } else if (parsedContent.colorType == PntsColorType::RGB565) {

    const std::span<const uint16_t> compressedColors(
        reinterpret_cast<const uint16_t*>(
            featureTableBinaryData.data() + color.byteOffset),
        pointsLength);
    std::span<glm::vec3> outColors(
        reinterpret_cast<glm::vec3*>(colorData.data()),
        pointsLength);

    for (size_t i = 0; i < pointsLength; i++) {
      const uint16_t compressedColor = compressedColors[i];
      glm::vec3 decompressedColor =
          glm::vec3(AttributeCompression::decodeRGB565(compressedColor));
      outColors[i] = srgbToLinear(decompressedColor);
    }
  }
}

void parseNormalsFromFeatureTableBinary(
    const std::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  CESIUM_ASSERT(parsedContent.normal.has_value());
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
    const std::span<const glm::u8vec2> encodedNormals(
        reinterpret_cast<const glm::u8vec2*>(
            featureTableBinaryData.data() + normal.byteOffset),
        pointsLength);

    std::span<glm::vec3> outNormals(
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
    const std::span<const std::byte>& featureTableBinaryData,
    PntsContent& parsedContent) {
  CESIUM_ASSERT(parsedContent.batchId.has_value());
  PntsSemantic& batchId = parsedContent.batchId.value();
  std::vector<std::byte>& batchIdData = batchId.data;
  if (batchIdData.size() > 0) {
    // If data isn't empty, it must have been decoded from Draco.
    return;
  }

  const uint32_t pointsLength = parsedContent.pointsLength;
  size_t batchIdsByteStride = sizeof(uint16_t);
  if (parsedContent.batchIdComponentType) {
    batchIdsByteStride = MetadataProperty::getSizeOfComponentType(
        parsedContent.batchIdComponentType.value());
  }
  const size_t batchIdsByteLength = pointsLength * batchIdsByteStride;
  batchIdData.resize(batchIdsByteLength);

  std::memcpy(
      batchIdData.data(),
      featureTableBinaryData.data() + batchId.byteOffset,
      batchIdsByteLength);
}

void parseFeatureTableBinary(
    const std::span<const std::byte>& featureTableBinaryData,
    rapidjson::Document& batchTableJson,
    const std::span<const std::byte>& batchTableBinaryData,
    PntsContent& parsedContent) {
  decodeDraco(
      featureTableBinaryData,
      batchTableJson,
      batchTableBinaryData,
      parsedContent);
  if (parsedContent.errors) {
    return;
  }

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

int32_t createBufferInGltf(Model& gltf, std::vector<std::byte>&& buffer) {
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
    const std::string& type) {
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
  int32_t bufferId =
      createBufferInGltf(gltf, std::move(parsedContent.position.data));
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
  CESIUM_ASSERT(parsedContent.color.has_value());
  PntsSemantic& color = parsedContent.color.value();

  const int64_t count = static_cast<int64_t>(parsedContent.pointsLength);
  int64_t byteStride = 0;
  const int32_t componentType = Accessor::ComponentType::FLOAT;
  std::string type;
  bool isTranslucent = false;

  if (parsedContent.colorType == PntsColorType::RGBA) {
    byteStride = static_cast<int64_t>(sizeof(glm::vec4));
    type = Accessor::Type::VEC4;
    isTranslucent = true;
  } else {
    byteStride = static_cast<int64_t>(sizeof(glm::vec3));
    type = Accessor::Type::VEC3;
  }

  const int64_t byteLength = static_cast<int64_t>(byteStride * count);
  int32_t bufferId = createBufferInGltf(gltf, std::move(color.data));
  int32_t bufferViewId =
      createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);
  int32_t accessorId =
      createAccessorInGltf(gltf, bufferViewId, componentType, count, type);

  MeshPrimitive& primitive = gltf.meshes[0].primitives[0];
  primitive.attributes.emplace("COLOR_0", accessorId);

  if (isTranslucent) {
    Material& material =
        gltf.materials[static_cast<uint32_t>(primitive.material)];
    material.alphaMode = Material::AlphaMode::BLEND;
  }
}

void addNormalsToGltf(PntsContent& parsedContent, Model& gltf) {
  CESIUM_ASSERT(parsedContent.normal.has_value());
  PntsSemantic& normal = parsedContent.normal.value();

  const int64_t count = static_cast<int64_t>(parsedContent.pointsLength);
  const int64_t byteStride = static_cast<int64_t>(sizeof(glm ::vec3));
  const int64_t byteLength = static_cast<int64_t>(byteStride * count);

  int32_t bufferId = createBufferInGltf(gltf, std::move(normal.data));
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
  CESIUM_ASSERT(parsedContent.batchId.has_value());
  PntsSemantic& batchId = parsedContent.batchId.value();

  const int64_t count = static_cast<int64_t>(parsedContent.pointsLength);
  int32_t componentType = Accessor::ComponentType::UNSIGNED_SHORT;
  if (parsedContent.batchIdComponentType) {
    switch (parsedContent.batchIdComponentType.value()) {
    case MetadataProperty::ComponentType::UNSIGNED_BYTE:
      componentType = Accessor::ComponentType::UNSIGNED_BYTE;
      break;
    case MetadataProperty::ComponentType::UNSIGNED_INT:
      componentType = Accessor::ComponentType::UNSIGNED_INT;
      break;
    case MetadataProperty::ComponentType::UNSIGNED_SHORT:
    default:
      componentType = Accessor::ComponentType::UNSIGNED_SHORT;
      break;
    }
    const int64_t byteStride =
        Accessor::computeByteSizeOfComponent(componentType);
    const int64_t byteLength = static_cast<int64_t>(byteStride * count);

    int32_t bufferId = createBufferInGltf(gltf, std::move(batchId.data));
    int32_t bufferViewId =
        createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);
    int32_t accessorId = createAccessorInGltf(
        gltf,
        bufferViewId,
        componentType,
        count,
        Accessor::Type::SCALAR);

    MeshPrimitive& primitive = gltf.meshes[0].primitives[0];
    // This will be renamed by BatchTableToGltfStructuralMetadata.
    primitive.attributes.emplace("_BATCHID", accessorId);
  }
}

void createGltfFromParsedContent(
    PntsContent& parsedContent,
    GltfConverterResult& result) {
  result.model.reset();
  Model& gltf = result.model.emplace();

  gltf.asset.version = "2.0";

  // Create a single node with a single mesh, with a single primitive.
  Node& node = gltf.nodes.emplace_back();
  std::memcpy(
      node.matrix.data(),
      &CesiumGeometry::Transforms::Z_UP_TO_Y_UP,
      sizeof(glm::dmat4));

  // Create a scene containing the node, and make it the default scene.
  Scene& scene = gltf.scenes.emplace_back();
  scene.nodes = {0};
  gltf.scene = 0;

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
    glm::vec4 materialColor(parsedContent.constantRgba.value());
    materialColor = srgbToLinear(materialColor / 255.0f);

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
    gltf.addExtensionUsed(
        CesiumGltf::ExtensionKhrMaterialsUnlit::ExtensionName);
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
    result.model->addExtensionRequired(
        CesiumGltf::ExtensionCesiumRTC::ExtensionName);

    glm::dvec3 rtcCenter = parsedContent.rtcCenter.value();
    cesiumRTC.center = {rtcCenter.x, rtcCenter.y, rtcCenter.z};
  }
}

void convertPntsContentToGltf(
    const std::span<const std::byte>& pntsBinary,
    const PntsHeader& header,
    uint32_t headerLength,
    GltfConverterResult& result) {
  if (header.featureTableJsonByteLength > 0 &&
      header.featureTableBinaryByteLength > 0) {
    PntsContent parsedContent;

    const std::span<const std::byte> featureTableJsonData =
        pntsBinary.subspan(headerLength, header.featureTableJsonByteLength);
    rapidjson::Document featureTableJson =
        parseFeatureTableJson(featureTableJsonData, parsedContent);
    if (parsedContent.errors) {
      result.errors.merge(parsedContent.errors);
      return;
    }

    // If the batch table contains the 3DTILES_draco_point_compression
    // extension, the compressed metdata properties will be included in the
    // feature table binary. Parse both JSONs first in case the extension is
    // there.
    const int64_t batchTableStart = headerLength +
                                    header.featureTableJsonByteLength +
                                    header.featureTableBinaryByteLength;
    rapidjson::Document batchTableJson;
    if (header.batchTableJsonByteLength > 0) {
      const std::span<const std::byte> batchTableJsonData = pntsBinary.subspan(
          static_cast<size_t>(batchTableStart),
          header.batchTableJsonByteLength);
      batchTableJson = parseBatchTableJson(batchTableJsonData, parsedContent);
    }

    const std::span<const std::byte> featureTableBinaryData =
        pntsBinary.subspan(
            static_cast<size_t>(
                headerLength + header.featureTableJsonByteLength),
            header.featureTableBinaryByteLength);

    std::span<const std::byte> batchTableBinaryData;
    if (header.batchTableBinaryByteLength > 0) {
      batchTableBinaryData = pntsBinary.subspan(
          static_cast<size_t>(
              batchTableStart + header.batchTableJsonByteLength),
          header.batchTableBinaryByteLength);
    }

    parseFeatureTableBinary(
        featureTableBinaryData,
        batchTableJson,
        batchTableBinaryData,
        parsedContent);

    if (parsedContent.errors) {
      result.errors.merge(parsedContent.errors);
      return;
    }

    createGltfFromParsedContent(parsedContent, result);

    if (!batchTableJson.IsObject() || batchTableJson.HasParseError() ||
        parsedContent.dracoMetadataHasErrors) {
      result.errors.merge(parsedContent.errors);
      return;
    }

    if (!parsedContent.dracoBatchTableBinary.empty()) {
      // If the point cloud has both compressed and uncompressed metadata
      // values, then dracoBatchTableBinary will contain both the original
      // batch table binary and the Draco decoded values.
      batchTableBinaryData =
          std::span<const std::byte>(parsedContent.dracoBatchTableBinary);
    }

    if (result.model) {
      result.errors.merge(BatchTableToGltfStructuralMetadata::convertFromPnts(
          featureTableJson,
          batchTableJson,
          batchTableBinaryData,
          result.model.value()));
    }
  }
}
} // namespace

CesiumAsync::Future<GltfConverterResult> PntsToGltfConverter::convert(
    const std::span<const std::byte>& pntsBinary,
    const CesiumGltfReader::GltfReaderOptions& /*options*/,
    const AssetFetcher& assetFetcher) {
  GltfConverterResult result;
  PntsHeader header;
  uint32_t headerLength = 0;
  parsePntsHeader(pntsBinary, header, headerLength, result);
  if (!result.errors) {
    convertPntsContentToGltf(pntsBinary, header, headerLength, result);
  }

  return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
}
} // namespace Cesium3DTilesContent
