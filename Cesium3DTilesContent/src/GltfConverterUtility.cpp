#include <Cesium3DTilesContent/GltfConverterUtility.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumUtility/ErrorList.h>

#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/matrix.hpp>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Cesium3DTilesContent {
namespace GltfConverterUtility {
using namespace CesiumGltf;

std::optional<uint32_t> parseOffsetForSemantic(
    const rapidjson::Document& document,
    const char* semantic,
    CesiumUtility::ErrorList& errorList) {
  const auto semanticIt = document.FindMember(semantic);
  if (semanticIt == document.MemberEnd() || !semanticIt->value.IsObject()) {
    return {};
  }
  const auto byteOffsetIt = semanticIt->value.FindMember("byteOffset");
  if (byteOffsetIt == semanticIt->value.MemberEnd() ||
      !isValue<uint32_t>(byteOffsetIt->value)) {
    errorList.emplaceError(
        std::string("Error parsing feature table, ") + semantic +
        "does not have valid byteOffset.");
    return {};
  }
  return getValue<uint32_t>(byteOffsetIt->value);
}

bool validateJsonArrayValues(
    const rapidjson::Value& arrayValue,
    uint32_t expectedLength,
    ValuePredicate predicate) {
  if (!arrayValue.IsArray()) {
    return false;
  }

  if (arrayValue.Size() != expectedLength) {
    return false;
  }

  for (rapidjson::SizeType i = 0; i < expectedLength; i++) {
    if (!std::invoke(predicate, arrayValue[i])) {
      return false;
    }
  }

  return true;
}

std::optional<glm::dvec3>
parseArrayValueDVec3(const rapidjson::Value& arrayValue) {
  if (validateJsonArrayValues(arrayValue, 3, &rapidjson::Value::IsNumber)) {
    return std::make_optional(glm::dvec3(
        arrayValue[0].GetDouble(),
        arrayValue[1].GetDouble(),
        arrayValue[2].GetDouble()));
  }
  return {};
}

std::optional<glm::dvec3>
parseArrayValueDVec3(const rapidjson::Document& document, const char* name) {
  const auto arrayIt = document.FindMember(name);
  if (arrayIt != document.MemberEnd()) {
    return parseArrayValueDVec3(arrayIt->value);
  }
  return {};
}

int32_t createBufferInGltf(Model& gltf, std::vector<std::byte> buffer) {
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

void applyRtcToNodes(Model& gltf, const glm::dvec3& rtc) {
  using namespace CesiumGltfContent;
  auto upToZ = GltfUtilities::applyGltfUpAxisTransform(gltf, glm::dmat4x4(1.0));
  auto rtcTransform = inverse(upToZ);
  rtcTransform = glm::translate(rtcTransform, rtc);
  rtcTransform = rtcTransform * upToZ;
  gltf.forEachRootNodeInScene(-1, [&](Model&, Node& node) {
    auto nodeTransform = GltfUtilities::getNodeTransform(node);
    if (nodeTransform) {
      nodeTransform = rtcTransform * *nodeTransform;
      GltfUtilities::setNodeTransform(node, *nodeTransform);
    }
  });
}

} // namespace GltfConverterUtility
} // namespace Cesium3DTilesContent
