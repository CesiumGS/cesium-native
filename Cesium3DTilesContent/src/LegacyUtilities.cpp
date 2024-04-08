#include <Cesium3DTilesContent/LegacyUtilities.h>

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Uri.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <string>

namespace Cesium3DTilesContent {
namespace LegacyUtilities {
using namespace CesiumGltf;

std::optional<uint32_t> parseOffset( const rapidjson::Document& document, const char* semantic, CesiumUtility::ErrorList& errorList) {
  const auto semanticIt = document.FindMember(semantic);
  if (semanticIt == document.MemberEnd() || !semanticIt->value.IsObject()) {
    return {};
  }
  const auto byteOffsetIt = semanticIt->value.FindMember("byteOffset");
  if (byteOffsetIt == semanticIt->value.MemberEnd() || !isValue<uint32_t>(byteOffsetIt->value)) {
    errorList.emplaceError(
        std::string("Error parsing feature table, ") + semantic + "does not have valid byteOffset.");
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

std::optional<glm::vec3> parseArrayValueVec3(const rapidjson::Value& arrayValue) {
  if (validateJsonArrayValues(arrayValue, 3, &rapidjson::Value::IsNumber)) {
    return std::make_optional(glm::vec3(arrayValue[0].GetFloat(),
                                        arrayValue[1].GetFloat(),
                                        arrayValue[2].GetFloat()));
  }
  return {};
}

std::optional<glm::vec3> parseArrayValueVec3(const rapidjson::Document& document, const char *name) {
  const auto arrayIt = document.FindMember(name);
  if (arrayIt != document.MemberEnd()) {
    return parseArrayValueVec3(arrayIt->value);
  }
  return {};
}

int32_t createBufferInGltf(Model& gltf) {
  size_t bufferId = gltf.buffers.size();
  Buffer& gltfBuffer = gltf.buffers.emplace_back();
  gltfBuffer.byteLength = 0;
  return static_cast<int32_t>(bufferId);
}

int32_t createBufferInGltf(Model& gltf, std::vector<std::byte>&& buffer) {
  int32_t bufferId = createBufferInGltf(gltf);
  Buffer& gltfBuffer = gltf.buffers[static_cast<uint32_t>(bufferId)];
  gltfBuffer.byteLength = static_cast<int32_t>(buffer.size());
  gltfBuffer.cesium.data = std::move(buffer);
  return bufferId;
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

}

CesiumAsync::Future<ByteResult> get(const ConverterSubprocessor& subprocessor, const std::string& relativeUrl)
{
  auto resolvedUrl = CesiumUtility::Uri::resolve(subprocessor.baseUrl, relativeUrl);
  return subprocessor.pAssetAccessor->get(subprocessor.asyncSystem, resolvedUrl, subprocessor.requestHeaders)
      .thenImmediately([asyncSystem = subprocessor.asyncSystem](std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
        const CesiumAsync::IAssetResponse* pResponse =  pCompletedRequest->response();
        ByteResult byteResult;
        const auto& url = pCompletedRequest->url();
        if (!pResponse) {
          byteResult.errorList.emplaceError(fmt::format("Did not receive a valid response for asset {}", url));
          return asyncSystem.createResolvedFuture(std::move(byteResult));
        }
        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          byteResult.errorList.emplaceError(fmt::format(
                                                "Received status code {} for asset {}",
                                                statusCode,
                                                url));
          return asyncSystem.createResolvedFuture(
              std::move(byteResult));
        }
        gsl::span<const std::byte> asset = pResponse->data();
        std::copy(asset.begin(), asset.end(), std::back_inserter(byteResult.bytes));
        return asyncSystem.createResolvedFuture(std::move(byteResult));
      });
}
}
