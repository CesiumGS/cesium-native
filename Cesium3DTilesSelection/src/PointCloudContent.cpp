#include "PointCloudContent.h"
#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "CesiumGeometry/AxisTransforms.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CesiumUtility/Tracing.h"
#include "upgradeBatchTableToFeatureMetadata.h"
#include <cstddef>
#include <rapidjson/document.h>
#include <stdexcept>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127 4018 4804)
#endif

#include "draco/attributes/point_attribute.h"
#include "draco/compression/decode.h"
#include "draco/core/decoder_buffer.h"
#include "draco/point_cloud/point_cloud.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Cesium3DTilesSelection {

struct PntsHeader {
  unsigned char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t featureTableJsonByteLength;
  uint32_t featureTableBinaryByteLength;
  uint32_t batchTableJsonByteLength;
  uint32_t batchTableBinaryByteLength;
};

namespace {

rapidjson::Document parseFeatureTable(
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumGltf::Model& gltf,
    const gsl::span<const std::byte>& featureTableJsonData,
    const gsl::span<const std::byte>& featureTableBinaryData) {
  rapidjson::Document document;
  document.Parse(
      reinterpret_cast<const char*>(featureTableJsonData.data()),
      featureTableJsonData.size());
  if (document.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        document.GetParseError(),
        document.GetErrorOffset());
    return document;
  }

  auto rtcIt = document.FindMember("RTC_CENTER");
  if (rtcIt != document.MemberEnd() && rtcIt->value.IsArray() &&
      rtcIt->value.Size() == 3 && rtcIt->value[0].IsNumber() &&
      rtcIt->value[1].IsNumber() && rtcIt->value[2].IsNumber()) {
    // Add the RTC_CENTER value to the glTF itself.
    rapidjson::Value& rtcValue = rtcIt->value;
    gltf.extras["RTC_CENTER"] = {
        rtcValue[0].GetDouble(),
        rtcValue[1].GetDouble(),
        rtcValue[2].GetDouble()};
  }

  uint32_t positionsOffset = 0;
  bool usingQuantizedPositions = false;
  glm::dvec3 quantizedVolumeOffset;
  glm::dvec3 quantizedVolumeScale;

  auto positionIt = document.FindMember("POSITION");
  auto positionQuantizedIt = document.FindMember("POSITION_QUANTIZED");

  if (positionIt != document.MemberEnd() && positionIt->value.IsObject()) {

    auto byteOffsetIt = positionIt->value.FindMember("byteOffset");

    if (byteOffsetIt != positionIt->value.MemberEnd() &&
        byteOffsetIt->value.IsUint()) {
      positionsOffset = byteOffsetIt->value.GetUint();
    }
  } else if (
      positionQuantizedIt != document.MemberEnd() &&
      positionQuantizedIt->value.IsObject()) {

    auto byteOffsetIt = positionQuantizedIt->value.FindMember("byteOffset");

    if (byteOffsetIt != positionQuantizedIt->value.MemberEnd() &&
        byteOffsetIt->value.IsUint()) {
      usingQuantizedPositions = true;
      positionsOffset = byteOffsetIt->value.GetUint();
      auto quantizedVolumeOffsetIt =
          document.FindMember("QUANTIZED_VOLUME_OFFSET");
      auto quantizedVolumeScaleIt =
          document.FindMember("QUANTIZED_VOLUME_SCALE");

      if (quantizedVolumeOffsetIt != document.MemberEnd() &&
          quantizedVolumeOffsetIt->value.IsArray() &&
          quantizedVolumeOffsetIt->value.Size() == 3 &&
          quantizedVolumeOffsetIt->value[0].IsDouble() &&
          quantizedVolumeOffsetIt->value[1].IsDouble() &&
          quantizedVolumeOffsetIt->value[2].IsDouble() &&
          quantizedVolumeScaleIt != document.MemberEnd() &&
          quantizedVolumeScaleIt->value.IsArray() &&
          quantizedVolumeScaleIt->value.Size() == 3 &&
          quantizedVolumeScaleIt->value[0].IsDouble() &&
          quantizedVolumeScaleIt->value[1].IsDouble() &&
          quantizedVolumeScaleIt->value[2].IsDouble()) {

        quantizedVolumeOffset = glm::dvec3(
            quantizedVolumeOffsetIt->value[0].GetDouble(),
            quantizedVolumeOffsetIt->value[1].GetDouble(),
            quantizedVolumeOffsetIt->value[2].GetDouble());
        quantizedVolumeScale = glm::dvec3(
            quantizedVolumeScaleIt->value[0].GetDouble(),
            quantizedVolumeScaleIt->value[1].GetDouble(),
            quantizedVolumeScaleIt->value[2].GetDouble());
      }
    }
  } else {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error parsing PNTS content, no POSITION semantic found.");

    return document;
  }

  uint32_t pointsLength = 0;
  auto pointsLengthIt = document.FindMember("POINTS_LENGTH");
  if (pointsLengthIt != document.MemberEnd() &&
      pointsLengthIt->value.IsUint()) {
    pointsLength = pointsLengthIt->value.GetUint();
    if (pointsLength <= 3) {
      SPDLOG_LOGGER_ERROR(
          pLogger,
          "Error parsing PNTS content, not enough points.");

      return document;
    }
  } else {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error parsing PNTS content, no POINTS_LENGTH found.");

    return document;
  }

  size_t positionsByteStride = sizeof(glm::vec3);
  size_t positionsBufferSize = pointsLength * positionsByteStride;

  std::vector<std::byte> outPositionsBuffer(positionsBufferSize);
  gsl::span<glm::vec3> outPositions(
      reinterpret_cast<glm::vec3*>(outPositionsBuffer.data()),
      pointsLength);

  bool usingColors = false;
  uint32_t colorsOffset = 0;

  // TODO: look for other color formats as well
  auto colorsIt = document.FindMember("RGB");
  if (colorsIt != document.MemberEnd() && colorsIt->value.IsObject()) {
    auto byteOffsetIt = colorsIt->value.FindMember("byteOffset");
    if (byteOffsetIt != colorsIt->value.MemberEnd() &&
        byteOffsetIt->value.IsUint()) {
      usingColors = true;
      colorsOffset = byteOffsetIt->value.GetUint();
    }
  }

  struct RGB24 {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  size_t colorsByteStride = sizeof(RGB24);
  size_t colorsBufferSize = pointsLength * colorsByteStride;

  std::vector<std::byte> outColorsBuffer(colorsBufferSize);
  gsl::span<RGB24> outColors(
      reinterpret_cast<RGB24*>(outColorsBuffer.data()),
      pointsLength);

  // check if the point cloud is draco-compressed
  bool usingDraco = false;

  auto extensionsIt = document.FindMember("extensions");
  if (extensionsIt != document.MemberEnd() && extensionsIt->value.IsObject()) {

    auto dracoExtensionIt =
        extensionsIt->value.FindMember("3DTILES_draco_point_compression");
    if (dracoExtensionIt != extensionsIt->value.MemberEnd() &&
        dracoExtensionIt->value.IsObject()) {

      auto propertiesIt = dracoExtensionIt->value.FindMember("properties");
      auto byteOffsetIt = dracoExtensionIt->value.FindMember("byteOffset");
      auto byteLengthIt = dracoExtensionIt->value.FindMember("byteLength");
      if (propertiesIt != dracoExtensionIt->value.MemberEnd() &&
          propertiesIt->value.IsObject() &&
          byteOffsetIt != dracoExtensionIt->value.MemberEnd() &&
          byteOffsetIt->value.IsUint64() &&
          byteLengthIt != dracoExtensionIt->value.MemberEnd() &&
          byteLengthIt->value.IsUint64()) {

        uint64_t byteOffset = byteOffsetIt->value.GetUint64();
        uint64_t byteLength = byteLengthIt->value.GetUint64();

        auto positionPropertyIt = propertiesIt->value.FindMember("POSITION");
        if (positionPropertyIt != propertiesIt->value.MemberEnd() &&
            positionPropertyIt->value.IsInt()) {

          int32_t positionProperty = positionPropertyIt->value.GetInt();

          std::optional<int32_t> colorProperty = std::nullopt;

          auto colorPropertyIt = propertiesIt->value.FindMember("RGB");
          if (colorPropertyIt != propertiesIt->value.MemberEnd() &&
              colorPropertyIt->value.IsInt()) {
            colorProperty = colorPropertyIt->value.GetInt();
          }

          draco::Decoder decoder;
          draco::DecoderBuffer buffer;
          buffer.Init(
              (char*)featureTableBinaryData.data() + byteOffset,
              byteLength);

          draco::StatusOr<std::unique_ptr<draco::PointCloud>> result =
              decoder.DecodePointCloudFromBuffer(&buffer);

          if (!result.ok()) {
            SPDLOG_LOGGER_ERROR(pLogger, "Error decoding draco point cloud.");

            return document;
          }

          const std::unique_ptr<draco::PointCloud>& pPointCloud =
              result.value();

          draco::PointAttribute* pPositionAttribute =
              pPointCloud->attribute(positionProperty);

          if (!pPositionAttribute ||
              pPositionAttribute->data_type() != draco::DT_FLOAT32 ||
              pPositionAttribute->num_components() != 3) {
            SPDLOG_LOGGER_ERROR(
                pLogger,
                "Draco decoded point cloud has an invalid position attribute.");

            return document;
          }

          draco::DataBuffer* decodedPositionBuffer =
              pPositionAttribute->buffer();
          int64_t decodedPositionByteOffset = pPositionAttribute->byte_offset();
          int64_t decodedPositionByteStride = pPositionAttribute->byte_stride();
          for (uint32_t i = 0; i < pointsLength; ++i) {
            outPositions[i] = *reinterpret_cast<const glm::vec3*>(
                decodedPositionBuffer->data() + decodedPositionByteOffset +
                decodedPositionByteStride * i);
          }

          if (colorProperty) {
            draco::PointAttribute* pColorAttribute =
                pPointCloud->attribute(*colorProperty);

            if (pColorAttribute) {
              draco::DataBuffer* decodedColorBuffer = pColorAttribute->buffer();
              int64_t decodedColorByteOffset = pColorAttribute->byte_offset();
              int64_t decodedColorByteStride = pColorAttribute->byte_stride();
              for (uint32_t i = 0; i < pointsLength; ++i) {
                outColors[i] = *reinterpret_cast<const RGB24*>(
                    decodedColorBuffer->data() + decodedColorByteOffset +
                    decodedColorByteStride * i);
              }
            }
          }
          usingDraco = true;
        }
      }
    }
  }

  if (!usingDraco) {
    if (usingQuantizedPositions) {
      gsl::span<const uint16_t> quantizedPositionComponents(
          reinterpret_cast<const uint16_t*>(
              featureTableBinaryData.data() + positionsOffset),
          3 * pointsLength);
      for (size_t i = 2; i < 3 * pointsLength; ++i) {
        glm::vec3 positionQuantized(
            quantizedPositionComponents[i - 2],
            quantizedPositionComponents[i - 1],
            quantizedPositionComponents[i]);

        // TODO: does this lose precision?
        outPositions[i / 3] =
            positionQuantized * glm::vec3(quantizedVolumeScale) / 65535.0f +
            glm::vec3(quantizedVolumeOffset);
      }
    } else {
      std::memcpy(
          outPositionsBuffer.data(),
          featureTableBinaryData.data() + positionsOffset,
          positionsBufferSize);
    }

    std::memcpy(
        outColorsBuffer.data(),
        featureTableBinaryData.data() + colorsOffset,
        colorsBufferSize);
  }

  size_t positionsBufferId = gltf.buffers.size();
  CesiumGltf::Buffer& positionsBuffer = gltf.buffers.emplace_back();
  positionsBuffer.byteLength = static_cast<int32_t>(positionsBufferSize);
  positionsBuffer.cesium.data = std::move(outPositionsBuffer);

  size_t positionsBufferViewId = gltf.bufferViews.size();
  CesiumGltf::BufferView& positionsBufferView = gltf.bufferViews.emplace_back();
  positionsBufferView.buffer = static_cast<int32_t>(positionsBufferId);
  positionsBufferView.byteLength = static_cast<int64_t>(positionsBufferSize);
  positionsBufferView.byteOffset = 0;
  positionsBufferView.byteStride = static_cast<int64_t>(positionsByteStride);
  positionsBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  size_t positionAccessorId = gltf.accessors.size();
  CesiumGltf::Accessor& positionsAccessor = gltf.accessors.emplace_back();
  positionsAccessor.bufferView = static_cast<int32_t>(positionsBufferViewId);
  positionsAccessor.byteOffset = 0;
  positionsAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  positionsAccessor.count = int64_t(pointsLength);
  positionsAccessor.type = CesiumGltf::Accessor::Type::VEC3;

  size_t colorsBufferId = gltf.buffers.size();
  CesiumGltf::Buffer& colorsBuffer = gltf.buffers.emplace_back();
  colorsBuffer.byteLength = static_cast<int32_t>(colorsBufferSize);
  colorsBuffer.cesium.data = std::move(outColorsBuffer);

  size_t colorsBufferViewId = gltf.bufferViews.size();
  CesiumGltf::BufferView& colorsBufferView = gltf.bufferViews.emplace_back();
  colorsBufferView.buffer = static_cast<int32_t>(colorsBufferId);
  colorsBufferView.byteLength = static_cast<int64_t>(colorsBufferSize);
  colorsBufferView.byteOffset = 0;
  colorsBufferView.byteStride = static_cast<int64_t>(colorsByteStride);
  colorsBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  size_t colorAccessorId = gltf.accessors.size();
  CesiumGltf::Accessor& colorAccessor = gltf.accessors.emplace_back();
  colorAccessor.bufferView = static_cast<int32_t>(colorsBufferViewId);
  colorAccessor.byteOffset = 0;
  colorAccessor.componentType =
      CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE;
  colorAccessor.count = int64_t(pointsLength);
  colorAccessor.type = CesiumGltf::Accessor::Type::VEC3;

  // Create a single node, with a single mesh, with a single primitive.
  size_t meshId = gltf.meshes.size();
  CesiumGltf::Mesh& mesh = gltf.meshes.emplace_back();
  CesiumGltf::MeshPrimitive& primitive = mesh.primitives.emplace_back();
  primitive.mode = CesiumGltf::MeshPrimitive::Mode::POINTS;
  primitive.attributes.emplace(
      "POSITION",
      static_cast<int32_t>(positionAccessorId));

  // TODO: send as regular vertex colors?
  primitive.attributes.emplace("RGB", static_cast<int32_t>(colorAccessorId));

  CesiumGltf::Node& node = gltf.nodes.emplace_back();
  std::memcpy(
      node.matrix.data(),
      &CesiumGeometry::AxisTransforms::Z_UP_TO_Y_UP,
      sizeof(glm::dmat4));
  node.mesh = static_cast<int32_t>(meshId);

  return document;
}

} // namespace

std::unique_ptr<TileContentLoadResult>
PointCloudContent::load(const TileContentLoadInput& input) {
  return load(input.pLogger, input.url, input.data);
}

std::unique_ptr<TileContentLoadResult> PointCloudContent::load(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& /*url*/,
    const gsl::span<const std::byte>& data) {
  // TODO: actually use the pnts payload
  if (data.size() < sizeof(PntsHeader)) {
    throw std::runtime_error(
        "The Point Cloud is invalid because it is too small to "
        "include a PNTS header.");
  }

  CESIUM_TRACE("Cesium3DTilesSelection::PointCloudContent::load");
  const PntsHeader* pHeader = reinterpret_cast<const PntsHeader*>(data.data());

  PntsHeader header = *pHeader;
  uint32_t headerLength = sizeof(PntsHeader);

  if (static_cast<uint32_t>(data.size()) < pHeader->byteLength) {
    throw std::runtime_error("The Point Cloud is invalid because the total "
                             "data available is less than the "
                             "size specified in its header.");
  }

  std::unique_ptr<TileContentLoadResult> pResult =
      std::make_unique<TileContentLoadResult>();

  if (header.featureTableJsonByteLength > 0) {
    pResult->model = CesiumGltf::Model();
    CesiumGltf::Model& gltf = *pResult->model;

    gsl::span<const std::byte> featureTableJsonData =
        data.subspan(headerLength, header.featureTableJsonByteLength);
    gsl::span<const std::byte> featureTableBinaryData = data.subspan(
        headerLength + header.featureTableJsonByteLength,
        header.featureTableBinaryByteLength);

    rapidjson::Document featureTable = parseFeatureTable(
        pLogger,
        gltf,
        featureTableJsonData,
        featureTableBinaryData);

    /*
        int64_t batchTableStart = headerLength +
       header.featureTableJsonByteLength + header.featureTableBinaryByteLength;
        int64_t batchTableLength =
            header.batchTableBinaryByteLength + header.batchTableJsonByteLength;

        if (batchTableLength > 0) {
          gsl::span<const std::byte> batchTableJsonData = data.subspan(
              static_cast<size_t>(batchTableStart),
              header.batchTableJsonByteLength);
          gsl::span<const std::byte> batchTableBinaryData = data.subspan(
              static_cast<size_t>(
                  batchTableStart + header.batchTableJsonByteLength),
              header.batchTableBinaryByteLength);

          rapidjson::Document batchTableJson;
          batchTableJson.Parse(
              reinterpret_cast<const char*>(batchTableJsonData.data()),
              batchTableJsonData.size());
          if (batchTableJson.HasParseError()) {
            SPDLOG_LOGGER_WARN(
                pLogger,
                "Error when parsing batch table JSON, error code {} at byte "
                "offset "
                "{}. Skip parsing metadata",
                batchTableJson.GetParseError(),
                batchTableJson.GetErrorOffset());
            return pResult;
          }

          upgradeBatchTableToFeatureMetadata(
              pLogger,
              gltf,
              featureTable,
              batchTableJson,
              batchTableBinaryData);
        }*/
  }

  return pResult;
}

} // namespace Cesium3DTilesSelection
