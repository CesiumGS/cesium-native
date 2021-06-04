#include "Instanced3DModelContent.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include <glm/vec3.hpp>
#include <cstddef>
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

  auto rtcIt = document.FindMember("RTC_CENTER");
  if (rtcIt != document.MemberEnd() && rtcIt->value.IsArray() &&
      rtcIt->value.Size() == 3 && rtcIt->value[0].IsDouble() &&
      rtcIt->value[1].IsDouble() && rtcIt->value[2].IsDouble()) {
    // Add the RTC_CENTER value to the glTF itself.
    rapidjson::Value& rtcValue = rtcIt->value;
    gltf.extras["RTC_CENTER"] = {
        rtcValue[0].GetDouble(),
        rtcValue[1].GetDouble(),
        rtcValue[2].GetDouble()};
  }

  uint32_t instancesLength = 0;
  auto instancesLengthIt =
      document.FindMember("INSTANCES_LENGTH");
  if (instancesLengthIt != document.MemberEnd() &&
      instancesLengthIt->value.IsUint()) {
    instancesLength = instancesLengthIt->value.GetUint();
  }

  // TODO: CATCH CASES WITH ILL FORMED I3DM
  // TODO: actually use a proper extension (KHR_gpu_mesh_instancing?)

  uint32_t positionsOffset = 0;
  bool usingPositions = false;
  bool usingQuantizedPositions = false;
  glm::dvec3 quantizedVolumeOffset;
  glm::dvec3 quantizedVolumeScale;

  auto positionIt = document.FindMember("POSITION");
  auto positionQuantizedIt = document.FindMember("POSITION_QUANTIZED");

  if (positionIt != document.MemberEnd() && 
      positionIt->value.IsUint()) {
    usingPositions = true;
    positionsOffset = positionIt->value.GetUint();
  } else if (positionQuantizedIt != document.MemberEnd() && 
             positionQuantizedIt->value.IsUint()) {
    usingPositions = true;
    usingQuantizedPositions = true;
    positionsOffset = positionQuantizedIt->value.GetUint();
    auto quantizedVolumeOffsetIt = document.FindMember("QUANTIZED_VOLUME_OFFSET");
    auto quantizedVolumeScaleIt = document.FindMember("QUANTIZED_VOLUME_SCALE");
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
        quantizedVolumeOffsetIt->value[2].GetDouble()
      );
      quantizedVolumeScale = glm::dvec3(
        quantizedVolumeScaleIt->value[0].GetDouble(),
        quantizedVolumeScaleIt->value[1].GetDouble(),
        quantizedVolumeScaleIt->value[2].GetDouble()
      );
    }
  }

  if (usingPositions) {
    size_t positionsByteStride = 3 * usingQuantizedPositions ? sizeof(uint16_t) : sizeof(float);
    size_t positionsBufferSize = instancesLength * positionsByteStride;

    size_t positionsBufferId = gltf.buffers.size();
    CesiumGltf::Buffer& positionsBuffer = gltf.buffers.emplace_back();
    positionsBuffer.cesium.data.resize(positionsBufferSize);
    std::memcpy(positionsBuffer.cesium.data.data(), 
                featureTableBinaryData.data() + positionsOffset, 
                positionsBufferSize);

    size_t positionsBufferViewId = gltf.bufferViews.size();
    CesiumGltf::BufferView& positionsBufferView = gltf.bufferViews.emplace_back();
    positionsBufferView.buffer = static_cast<int32_t>(positionsBufferId);
    positionsBufferView.byteLength = positionsBufferSize;
    positionsBufferView.byteOffset = 0;
    positionsBufferView.byteStride = positionsByteStride;
    positionsBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    size_t positionsAccessorId = gltf.accessors.size();
    CesiumGltf::Accessor& positionsAccessor = gltf.accessors.emplace_back();
    positionsAccessor.bufferView = static_cast<int32_t>(positionsBufferViewId);
    positionsAccessor.byteOffset = 0;
    positionsAccessor.componentType = 
        usingQuantizedPositions ? 
        CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT :
        CesiumGltf::Accessor::ComponentType::FLOAT;
    positionsAccessor.count = int64_t(instancesLength);
    positionsAccessor.type = CesiumGltf::Accessor::Type::VEC3;

    gltf.extras.emplace(
      usingQuantizedPositions ? 
        "INSTANCE_QUANTIZED_POSITIONS" : 
        "INSTANCE_POSITIONS", 
      positionsAccessorId);
  }

  uint32_t normalUpOffset = 0; 
  uint32_t normalRightOffset = 0;

  auto normalUpIt = document.FindMember("NORMAL_UP");
  auto normalRightIt = document.FindMember("NORMAL_RIGHT");
  auto normalUpOct32pIt = document.FindMember("NORMAL_UP_OCT32P");
  auto normalRightOct32pIt = document.FindMember("NORMAL_RIGHT_OCT32P");

  bool usingNormals = false;
  bool usingOct32Normals = false;

  if (normalUpIt != document.MemberEnd() && normalUpIt->value.IsUint() &&
      normalRightIt != document.MemberEnd() && normalRightIt->value.IsUint()) {
    usingNormals = true;
    normalUpOffset = normalUpIt->value.GetUint();
    normalRightOffset = normalRightIt->value.GetUint();
  } else if (normalUpOct32pIt != document.MemberEnd() &&
             normalUpOct32pIt->value.IsUint() &&
             normalRightOct32pIt != document.MemberEnd() &&
             normalRightOct32pIt->value.IsUint()) {
    usingNormals = true;
    usingOct32Normals = true;
    normalUpOffset = normalUpOct32pIt->value.GetUint();
    normalRightOffset = normalRightOct32pIt->value.GetUint();
  }

  if (usingNormals) {
    size_t normalByteStride = usingOct32Normals ? 2 * sizeof(uint16_t) : 3 * sizeof(float);
    size_t normalBufferSize = instancesLength * normalByteStride;
    
    size_t normalUpBufferId = gltf.buffers.size();
    CesiumGltf::Buffer& normalUpBuffer = gltf.buffers.emplace_back();
    normalUpBuffer.cesium.data.resize(normalBufferSize);
    std::memcpy(normalUpBuffer.cesium.data.data(), featureTableBinaryData.data() + normalUpOffset, normalBufferSize);

    size_t normalUpBufferViewId = gltf.bufferViews.size();
    CesiumGltf::BufferView& normalUpBufferView = gltf.bufferViews.emplace_back();
    normalUpBufferView.buffer = static_cast<int32_t>(normalUpBufferId);
    normalUpBufferView.byteLength = normalBufferSize;
    normalUpBufferView.byteOffset = 0;
    normalUpBufferView.byteStride = normalByteStride;
    normalUpBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    size_t normalUpAccessorId = gltf.accessors.size();
    CesiumGltf::Accessor& normalUpAccessor = gltf.accessors.emplace_back();
    normalUpAccessor.bufferView = static_cast<int32_t>(normalUpBufferViewId);
    normalUpAccessor.byteOffset = 0;
    normalUpAccessor.componentType = 
        usingOct32Normals ? 
        CesiumGltf::Accessor::ComponentType::SHORT :
        CesiumGltf::Accessor::ComponentType::FLOAT;
    normalUpAccessor.count = int64_t(instancesLength);
    normalUpAccessor.type = 
        usingOct32Normals ? 
        CesiumGltf::Accessor::Type::VEC2 :
        CesiumGltf::Accessor::Type::VEC3;

    gltf.extras.emplace(
      usingOct32Normals ? "INSTANCE_NORMAL_UP_OCT32P" : "INSTANCE_NORMAL_UP", 
      normalUpAccessorId); 

    size_t normalRightBufferId = gltf.buffers.size();
    CesiumGltf::Buffer& normalRightBuffer = gltf.buffers.emplace_back();
    normalRightBuffer.cesium.data.resize(normalBufferSize);
    std::memcpy(normalRightBuffer.cesium.data.data(), featureTableBinaryData.data() + normalRightOffset, normalBufferSize);

    size_t normalRightBufferViewId = gltf.bufferViews.size();
    CesiumGltf::BufferView& normalRightBufferView = gltf.bufferViews.emplace_back();
    normalRightBufferView.buffer = static_cast<int32_t>(normalRightBufferId);
    normalRightBufferView.byteLength = normalBufferSize;
    normalRightBufferView.byteOffset = 0;
    normalRightBufferView.byteStride = normalByteStride;
    normalRightBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    size_t normalRightAccessorId = gltf.accessors.size();
    CesiumGltf::Accessor& normalRightAccessor = gltf.accessors.emplace_back();
    normalRightAccessor.bufferView = static_cast<int32_t>(normalRightBufferViewId);
    normalRightAccessor.byteOffset = 0;
    normalRightAccessor.componentType = 
        usingOct32Normals ? 
        CesiumGltf::Accessor::ComponentType::SHORT :
        CesiumGltf::Accessor::ComponentType::FLOAT;
    normalRightAccessor.count = int64_t(instancesLength);
    normalRightAccessor.type = 
        usingOct32Normals ? 
        CesiumGltf::Accessor::Type::VEC2 :
        CesiumGltf::Accessor::Type::VEC3;

    gltf.extras.emplace(
      usingOct32Normals ? "INSTANCE_NORMAL_RIGHT_OCT32P" : "INSTANCE_NORMAL_RIGHT", 
      normalRightAccessorId); 
  }
}

} // namespace

CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
Instanced3DModelContent::load(const CesiumAsync::AsyncSystem& asyncSystem, const TileContentLoadInput& input) {
  return asyncSystem.createResolvedFuture(load(input.pLogger, input.url, input.data));
}

std::unique_ptr<TileContentLoadResult> Instanced3DModelContent::load(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const gsl::span<const std::byte>& data) {
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

  if (header.gltfFormat) {
    gsl::span<const std::byte> glbData =
        data.subspan(gltfStart, gltfEnd - gltfStart);
    std::unique_ptr<TileContentLoadResult> pResult =
        GltfContent::load(pLogger, url, glbData);

    if (pResult->model && header.featureTableJsonByteLength > 0) {
      CesiumGltf::Model& gltf = pResult->model.value();
      gsl::span<const std::byte> featureTableJsonData =
          data.subspan(headerLength, header.featureTableJsonByteLength);
      gsl::span<const std::byte> featureTableBinaryData = 
          data.subspan(headerLength + header.featureTableJsonByteLength, 
                       header.featureTableBinaryByteLength);
      parseFeatureTable(pLogger, gltf, featureTableJsonData, featureTableBinaryData);
    }

    return pResult;
  }

  std::string externalGltfUri(reinterpret_cast<char const*>(data.data() + gltfStart), gltfEnd - gltfStart);
  
  SPDLOG_LOGGER_ERROR(
    pLogger,
    "EXTERNAL GLTF: {}",
    externalGltfUri);

  // TODO: actually support gltf from URI source (also support pointing to
  // deferred asset)
  return nullptr;
}

} // namespace Cesium3DTiles
