#include "Instanced3DModelContent.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeometry/Axis.h"
#include "CesiumGltf/EXT_mesh_gpu_instancing.h"
#include "CesiumUtility/JsonValue.h"
#include "CesiumUtility/Uri.h"
#include <cstddef>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <rapidjson/document.h>
#include <stdexcept>

namespace Cesium3DTiles {

struct I3dmHeader {
  unsigned char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t featureTableJsonByteLength;
  uint32_t featureTableBinaryByteLength;
  uint32_t batchTableJsonByteLength;
  uint32_t batchTableBinaryByteLength;
  uint32_t gltfFormat;
};

namespace {

void parseFeatureTable(
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumGltf::Model& gltf,
    const CesiumGeometry::Axis& gltfUpAxis,
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
    return;
  }

  GltfContent::applyGltfUpTransformToNodes(gltf, gltfUpAxis);

  auto rtcIt = document.FindMember("RTC_CENTER");
  if (rtcIt != document.MemberEnd() && rtcIt->value.IsArray() &&
      rtcIt->value.Size() == 3 && rtcIt->value[0].IsDouble() &&
      rtcIt->value[1].IsDouble() && rtcIt->value[2].IsDouble()) {
    rapidjson::Value& rtcValue = rtcIt->value;
    glm::dvec3 rtcCenter(
        rtcValue[0].GetDouble(),
        rtcValue[1].GetDouble(),
        rtcValue[2].GetDouble());

    GltfContent::applyRtcCenterToNodes(gltf, rtcCenter);
  }

  uint32_t instancesLength = 0;
  auto instancesLengthIt = document.FindMember("INSTANCES_LENGTH");
  if (instancesLengthIt != document.MemberEnd() &&
      instancesLengthIt->value.IsUint()) {
    instancesLength = instancesLengthIt->value.GetUint();
    gltf.extras.emplace("INSTANCES_LENGTH", instancesLength);
  }

  size_t translationAccessorId = 0;
  size_t rotationsAccessorId = 0;
  // size_t scalesAccessorId = 0;

  // TODO: CATCH CASES WITH ILL FORMED I3DM

  uint32_t positionsOffset = 0;
  bool usingPositions = false;
  bool usingQuantizedPositions = false;
  glm::dvec3 quantizedVolumeOffset;
  glm::dvec3 quantizedVolumeScale;

  auto positionIt = document.FindMember("POSITION");
  auto positionQuantizedIt = document.FindMember("POSITION_QUANTIZED");

  if (positionIt != document.MemberEnd() && positionIt->value.IsObject()) {

    auto byteOffsetIt = positionIt->value.FindMember("byteOffset");

    if (byteOffsetIt != document.MemberEnd() && byteOffsetIt->value.IsUint()) {
      usingPositions = true;
      positionsOffset = byteOffsetIt->value.GetUint();
    }
  } else if (
      positionQuantizedIt != document.MemberEnd() &&
      positionQuantizedIt->value.IsObject()) {

    auto byteOffsetIt = positionQuantizedIt->value.FindMember("byteOffset");

    if (byteOffsetIt != document.MemberEnd() && byteOffsetIt->value.IsUint()) {
      usingPositions = true;
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
  }

  if (usingPositions) {
    size_t positionsByteStride =
        3 * (usingQuantizedPositions ? sizeof(uint16_t) : sizeof(float));
    size_t positionsBufferSize = instancesLength * positionsByteStride;

    size_t positionsBufferId = gltf.buffers.size();
    CesiumGltf::Buffer& positionsBuffer = gltf.buffers.emplace_back();
    positionsBuffer.cesium.data.resize(positionsBufferSize);
    std::memcpy(
        positionsBuffer.cesium.data.data(),
        featureTableBinaryData.data() + positionsOffset,
        positionsBufferSize);

    size_t positionsBufferViewId = gltf.bufferViews.size();
    CesiumGltf::BufferView& positionsBufferView =
        gltf.bufferViews.emplace_back();
    positionsBufferView.buffer = static_cast<int32_t>(positionsBufferId);
    positionsBufferView.byteLength = static_cast<int64_t>(positionsBufferSize);
    positionsBufferView.byteOffset = 0;
    positionsBufferView.byteStride = positionsByteStride;
    positionsBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    translationAccessorId = gltf.accessors.size();
    CesiumGltf::Accessor& positionsAccessor = gltf.accessors.emplace_back();
    positionsAccessor.bufferView = static_cast<int32_t>(positionsBufferViewId);
    positionsAccessor.byteOffset = 0;
    positionsAccessor.componentType =
        usingQuantizedPositions
            ? CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT
            : CesiumGltf::Accessor::ComponentType::FLOAT;
    positionsAccessor.count = int64_t(instancesLength);
    positionsAccessor.type = CesiumGltf::Accessor::Type::VEC3;
    // TODO: convey whether positions are quantized, or should they be converted
    // to normal positions on this side?
  }

  uint32_t normalUpOffset = 0;
  uint32_t normalRightOffset = 0;

  auto normalUpIt = document.FindMember("NORMAL_UP");
  auto normalRightIt = document.FindMember("NORMAL_RIGHT");
  auto normalUpOct32pIt = document.FindMember("NORMAL_UP_OCT32P");
  auto normalRightOct32pIt = document.FindMember("NORMAL_RIGHT_OCT32P");

  bool usingNormals = false;
  bool usingOct32Normals = false;

  if (normalUpIt != document.MemberEnd() && normalUpIt->value.IsObject() &&
      normalRightIt != document.MemberEnd() &&
      normalRightIt->value.IsObject()) {

    auto normalUpOffsetIt = normalUpIt->value.FindMember("byteOffset");
    auto normalRightOffsetIt = normalRightIt->value.FindMember("byteOffset");

    if (normalUpOffsetIt != document.MemberEnd() &&
        normalUpOffsetIt->value.IsUint() &&
        normalRightOffsetIt != document.MemberEnd() &&
        normalRightOffsetIt->value.IsUint()) {
      usingNormals = true;
      normalUpOffset = normalUpOffsetIt->value.GetUint();
      normalRightOffset = normalRightOffsetIt->value.GetUint();
    }
  } else if (
      normalUpOct32pIt != document.MemberEnd() &&
      normalUpOct32pIt->value.IsObject() &&
      normalRightOct32pIt != document.MemberEnd() &&
      normalRightOct32pIt->value.IsObject()) {

    auto normalUpOffsetIt = normalUpOct32pIt->value.FindMember("byteOffset");
    auto normalRightOffsetIt =
        normalRightOct32pIt->value.FindMember("byteOffset");

    if (normalUpOffsetIt != document.MemberEnd() &&
        normalUpOffsetIt->value.IsUint() &&
        normalRightOffsetIt != document.MemberEnd() &&
        normalRightOffsetIt->value.IsUint()) {
      usingNormals = true;
      usingOct32Normals = true;
      normalUpOffset = normalUpOffsetIt->value.GetUint();
      normalRightOffset = normalRightOffsetIt->value.GetUint();
    }
  }

  if (usingNormals) {
    // convert to quaternion buffer
    size_t rotationByteStride = sizeof(glm::quat);
    size_t rotationBufferSize = instancesLength * rotationByteStride;

    size_t rotationBufferId = gltf.buffers.size();
    CesiumGltf::Buffer& rotationBuffer = gltf.buffers.emplace_back();
    rotationBuffer.cesium.data.resize(rotationBufferSize);
    gsl::span<glm::quat> rotationSpan(
        reinterpret_cast<glm::quat*>(rotationBuffer.cesium.data.data()),
        instancesLength);

    if (usingOct32Normals) {

    } else {
      gsl::span<const glm::vec3> normalUpBuffer(
          reinterpret_cast<const glm::vec3*>(
              featureTableBinaryData.data() + normalUpOffset),
          instancesLength);
      gsl::span<const glm::vec3> normalRightBuffer(
          reinterpret_cast<const glm::vec3*>(
              featureTableBinaryData.data() + normalRightOffset),
          instancesLength);

      for (size_t i = 0; i < instancesLength; ++i) {
        const glm::vec3& normalUp = normalUpBuffer[i];
        const glm::vec3& normalRight = normalRightBuffer[i];

        glm::mat3 rotationMatrix(
            normalRight,
            normalUp,
            glm::normalize(glm::cross(normalRight, normalUp)));

        rotationSpan[i] = glm::quat(rotationMatrix);
      }
    }

    size_t rotationBufferViewId = gltf.bufferViews.size();
    CesiumGltf::BufferView& rotationBufferView =
        gltf.bufferViews.emplace_back();
    rotationBufferView.buffer = static_cast<int32_t>(rotationBufferId);
    rotationBufferView.byteLength = static_cast<int64_t>(rotationBufferSize);
    rotationBufferView.byteOffset = 0;
    rotationBufferView.byteStride = rotationByteStride;
    rotationBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    rotationsAccessorId = gltf.accessors.size();
    CesiumGltf::Accessor& rotationAccessor = gltf.accessors.emplace_back();
    rotationAccessor.bufferView = static_cast<int32_t>(rotationBufferViewId);
    rotationAccessor.byteOffset = 0;
    rotationAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
    rotationAccessor.count = int64_t(instancesLength);
    rotationAccessor.type = CesiumGltf::Accessor::Type::VEC4;
  }

  gltf.extensionsUsed.push_back("EXT_mesh_gpu_instancing");
  gltf.extensionsRequired.push_back("EXT_mesh_gpu_instancing");

  for (CesiumGltf::Node& node : gltf.nodes) {

    CesiumGltf::EXT_mesh_gpu_instancing instancingExtension;
    instancingExtension.attributes.emplace(
        "TRANSLATION",
        static_cast<int32_t>(translationAccessorId));
    instancingExtension.attributes.emplace(
        "ROTATION",
        static_cast<int32_t>(rotationsAccessorId));

    node.extensions.emplace("EXT_mesh_gpu_instancing", instancingExtension);
  }
}

} // namespace

CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
Instanced3DModelContent::load(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::vector<std::pair<std::string, std::string>>& requestHeaders,
    const TileContentLoadInput& input) {

  const std::shared_ptr<spdlog::logger>& pLogger = input.pLogger;
  const std::string& url = input.url;
  const gsl::span<const std::byte>& data = input.data;

  // TODO: actually use the i3dm payload
  if (data.size() < sizeof(I3dmHeader)) {
    throw std::runtime_error("The I3DM is invalid because it is too small to "
                             "include a I3DM header.");
  }

  const I3dmHeader* pHeader = reinterpret_cast<const I3dmHeader*>(data.data());

  I3dmHeader header = *pHeader;
  uint32_t headerLength = sizeof(I3dmHeader);

  if (static_cast<uint32_t>(data.size()) < pHeader->byteLength) {
    throw std::runtime_error(
        "The I3DM is invalid because the total data available is less than the "
        "size specified in its header.");
  }

  uint32_t gltfStart = headerLength + header.featureTableJsonByteLength +
                       header.featureTableBinaryByteLength +
                       header.batchTableJsonByteLength +
                       header.batchTableBinaryByteLength;
  uint32_t gltfEnd = header.byteLength;

  if (gltfEnd <= gltfStart) {
    throw std::runtime_error("The I3DM is invalid because the start of the "
                             "glTF model is after the end of the entire I3DM.");
  }

  gsl::span<const std::byte> featureTableJsonData =
      data.subspan(headerLength, header.featureTableJsonByteLength);
  gsl::span<const std::byte> featureTableBinaryData = data.subspan(
      headerLength + header.featureTableJsonByteLength,
      header.featureTableBinaryByteLength);

  if (header.gltfFormat) {
    gsl::span<const std::byte> glbData =
        data.subspan(gltfStart, gltfEnd - gltfStart);
    std::unique_ptr<TileContentLoadResult> pResult =
        GltfContent::load(pLogger, url, glbData);

    if (pResult->model) {
      CesiumGltf::Model& gltf = pResult->model.value();
      parseFeatureTable(
          pLogger,
          gltf,
          input.gltfUpAxis,
          featureTableJsonData,
          featureTableBinaryData);
    }

    return asyncSystem.createResolvedFuture(std::move(pResult));
  }

  std::string externalGltfUriRelative(
      reinterpret_cast<char const*>(data.data() + gltfStart),
      gltfEnd - gltfStart);

  std::string externalGltfUri =
      CesiumUtility::Uri::resolve(url, externalGltfUriRelative, false);

  SPDLOG_LOGGER_ERROR(pLogger, "EXTERNAL GLTF: {}", externalGltfUri);

  // TODO: actually support gltf from URI source (also support pointing to
  // deferred asset)
  return pAssetAccessor
      ->requestAsset(asyncSystem, externalGltfUri, requestHeaders)
      .thenInWorkerThread(
          [pLogger,
           externalGltfUri,
           gltfUpAxis = input.gltfUpAxis,
           featureTableJsonData,
           featureTableBinaryData](
              const std::shared_ptr<CesiumAsync::IAssetRequest>& pRequest) {
            const CesiumAsync::IAssetResponse* pResponse = pRequest->response();

            if (pResponse) {
              std::unique_ptr<TileContentLoadResult> pResult =
                  GltfContent::load(
                      pLogger,
                      externalGltfUri,
                      pResponse->data());

              if (pResult->model) {
                parseFeatureTable(
                    pLogger,
                    *pResult->model,
                    gltfUpAxis,
                    featureTableJsonData,
                    featureTableBinaryData);
              }

              return pResult;
            }

            return std::unique_ptr<TileContentLoadResult>(nullptr);
          });
  //.catchInMainThread()
}
} // namespace Cesium3DTiles
