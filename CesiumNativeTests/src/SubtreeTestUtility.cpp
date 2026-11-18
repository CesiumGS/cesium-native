#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumNativeTests/SubtreeTestUtility.h>

#include <libmorton/morton.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace Cesium3DTilesContent;

namespace CesiumNativeTests {
namespace {
struct NeedsAvailabilityBuffer {
  bool operator()(bool) { return false; }
  bool operator()(const std::vector<CesiumGeometry::QuadtreeTileID>&) {
    return true;
  }
};

struct GetAvailabilityView {
  std::span<std::byte> buffer;
  bool isSubtreeBuffer;

  SubtreeAvailability::AvailabilityView operator()(bool constant) {
    return SubtreeAvailability::SubtreeConstantAvailability{constant};
  }

  SubtreeAvailability::AvailabilityView
  operator()(const std::vector<CesiumGeometry::QuadtreeTileID>& availableIDs) {
    if (isSubtreeBuffer) {
      for (const auto& subtreeID : availableIDs) {
        markSubtreeAvailable(subtreeID, buffer);
      }
    } else {
      for (const auto& tileID : availableIDs) {
        markTileAvailable(tileID, buffer);
      }
    }

    return SubtreeAvailability::SubtreeBufferViewAvailability{buffer};
  }
};

} // namespace

SubtreeContent::SubtreeContent(
    uint32_t maxSubtreeLevel,
    const Input& tileAvailabilities,
    const Input& subtreeAvailabilities)
    : buffers(),
      tileAvailability(),
      subtreeAvailability(),
      contentAvailability() {
  this->initialize(maxSubtreeLevel, tileAvailabilities, subtreeAvailabilities);
}

void SubtreeContent::initialize(
    uint32_t maxSubtreeLevel,
    const Input& tileAvailabilities,
    const Input& subtreeAvailabilities) {
  bool needsTileAvailabilityBuffer =
      std::visit(NeedsAvailabilityBuffer{}, tileAvailabilities);
  bool needsSubtreeAvailabilityBuffer =
      std::visit(NeedsAvailabilityBuffer{}, subtreeAvailabilities);

  // Create and populate the availability buffers.
  uint64_t numTiles = getTotalNumberOfTilesForQuadtree(maxSubtreeLevel);
  uint64_t maxSubtreeTiles = uint64_t(1) << (2 * (maxSubtreeLevel));

  uint64_t bufferSize = needsTileAvailabilityBuffer
                            ? static_cast<uint64_t>(std::ceil(
                                  static_cast<double>(numTiles) / 8.0))
                            : 0;
  uint64_t subtreeBufferSize =
      needsSubtreeAvailabilityBuffer
          ? static_cast<uint64_t>(
                std::ceil(static_cast<double>(maxSubtreeTiles) / 8.0))
          : 0;

  this->buffers = std::vector<std::byte>(
      size_t(bufferSize + bufferSize + subtreeBufferSize));

  std::span<std::byte> contentAvailabilityBuffer(
      this->buffers.data(),
      size_t(bufferSize));
  std::span<std::byte> tileAvailabilityBuffer(
      this->buffers.data() + bufferSize,
      size_t(bufferSize));
  std::span<std::byte> subtreeAvailabilityBuffer(
      this->buffers.data() + bufferSize + bufferSize,
      size_t(subtreeBufferSize));

  this->tileAvailability = std::visit(
      GetAvailabilityView{tileAvailabilityBuffer, false},
      tileAvailabilities);
  this->contentAvailability = std::visit(
      GetAvailabilityView{contentAvailabilityBuffer, false},
      tileAvailabilities);
  this->subtreeAvailability = std::visit(
      GetAvailabilityView{subtreeAvailabilityBuffer, true},
      subtreeAvailabilities);
}

rapidjson::Document SubtreeContent::toJson(const std::string& bufferUrl) const {
  rapidjson::Document subtreeJson;
  subtreeJson.SetObject();

  bool hasTileAvailabilityBufferView = false;
  bool hasContentAvailabilityBufferView = false;
  bool hasSubtreeAvailabilityBufferView = false;

  // create buffers and buffer views, if necessary
  if (!this->buffers.empty()) {
    rapidjson::Value bufferObj(rapidjson::kObjectType);
    bufferObj.AddMember(
        "byteLength",
        uint64_t(this->buffers.size()),
        subtreeJson.GetAllocator());

    if (!bufferUrl.empty()) {
      rapidjson::Value uriStr(rapidjson::kStringType);
      uriStr.SetString(
          bufferUrl.c_str(),
          static_cast<rapidjson::SizeType>(bufferUrl.size()),
          subtreeJson.GetAllocator());
      bufferObj.AddMember("uri", std::move(uriStr), subtreeJson.GetAllocator());
    }

    rapidjson::Value buffersArray(rapidjson::kArrayType);
    buffersArray.GetArray().PushBack(
        std::move(bufferObj),
        subtreeJson.GetAllocator());

    subtreeJson.AddMember(
        "buffers",
        std::move(buffersArray),
        subtreeJson.GetAllocator());
    rapidjson::Value bufferViewsArray(rapidjson::kArrayType);

    auto addBufferViewIfNeeded =
        [&subtreeJson, &bufferViewsArray](
            const SubtreeAvailability::AvailabilityView& view,
            const std::vector<std::byte>& data,
            bool& result) {
          const auto* pBufferView =
              std::get_if<SubtreeAvailability::SubtreeBufferViewAvailability>(
                  &view);
          if (pBufferView) {
            rapidjson::Value bufferView(rapidjson::kObjectType);
            bufferView.AddMember(
                "buffer",
                uint64_t(0),
                subtreeJson.GetAllocator());
            bufferView.AddMember(
                "byteOffset",
                uint64_t(pBufferView->view.data() - data.data()),
                subtreeJson.GetAllocator());
            bufferView.AddMember(
                "byteLength",
                uint64_t(pBufferView->view.size()),
                subtreeJson.GetAllocator());
            bufferViewsArray.GetArray().PushBack(
                std::move(bufferView),
                subtreeJson.GetAllocator());
          }
          result = (pBufferView != nullptr);
        };

    addBufferViewIfNeeded(
        this->tileAvailability,
        this->buffers,
        hasTileAvailabilityBufferView);
    addBufferViewIfNeeded(
        this->contentAvailability,
        this->buffers,
        hasContentAvailabilityBufferView);
    addBufferViewIfNeeded(
        this->subtreeAvailability,
        this->buffers,
        hasSubtreeAvailabilityBufferView);

    subtreeJson.AddMember(
        "bufferViews",
        std::move(bufferViewsArray),
        subtreeJson.GetAllocator());
  }

  int32_t bufferViewIndex = 0;

  // create tileAvailability field
  rapidjson::Value tileAvailabilityObj(rapidjson::kObjectType);
  if (hasTileAvailabilityBufferView) {
    tileAvailabilityObj.AddMember(
        "bitstream",
        bufferViewIndex++,
        subtreeJson.GetAllocator());
  } else {
    const auto* pTileAvailabilityConstant =
        std::get_if<SubtreeAvailability::SubtreeConstantAvailability>(
            &this->tileAvailability);
    assert(pTileAvailabilityConstant);
    tileAvailabilityObj.AddMember(
        "constant",
        pTileAvailabilityConstant->constant ? 1 : 0,
        subtreeJson.GetAllocator());
  }
  subtreeJson.AddMember(
      "tileAvailability",
      std::move(tileAvailabilityObj),
      subtreeJson.GetAllocator());

  // create contentAvailability field
  rapidjson::Value contentAvailabilityObj(rapidjson::kObjectType);
  if (hasContentAvailabilityBufferView) {
    contentAvailabilityObj.AddMember(
        "bitstream",
        bufferViewIndex++,
        subtreeJson.GetAllocator());
  } else {
    const auto* pContentAvailabilityConstant =
        std::get_if<SubtreeAvailability::SubtreeConstantAvailability>(
            &this->contentAvailability);
    assert(pContentAvailabilityConstant);
    contentAvailabilityObj.AddMember(
        "constant",
        pContentAvailabilityConstant->constant ? 1 : 0,
        subtreeJson.GetAllocator());
  }

  rapidjson::Value contentAvailabilityArray(rapidjson::kArrayType);
  contentAvailabilityArray.GetArray().PushBack(
      std::move(contentAvailabilityObj),
      subtreeJson.GetAllocator());

  subtreeJson.AddMember(
      "contentAvailability",
      std::move(contentAvailabilityArray),
      subtreeJson.GetAllocator());

  // create childSubtreeAvailability
  rapidjson::Value subtreeAvailabilityObj(rapidjson::kObjectType);
  if (hasSubtreeAvailabilityBufferView) {
    subtreeAvailabilityObj.AddMember(
        "bitstream",
        bufferViewIndex++,
        subtreeJson.GetAllocator());
  } else {
    const auto* pSubtreeAvailabilityConstant =
        std::get_if<SubtreeAvailability::SubtreeConstantAvailability>(
            &this->subtreeAvailability);
    assert(pSubtreeAvailabilityConstant);
    subtreeAvailabilityObj.AddMember(
        "constant",
        pSubtreeAvailabilityConstant->constant ? 1 : 0,
        subtreeJson.GetAllocator());
  }
  subtreeJson.AddMember(
      "childSubtreeAvailability",
      std::move(subtreeAvailabilityObj),
      subtreeJson.GetAllocator());

  return subtreeJson;
}

std::vector<std::byte> SubtreeContent::toBinary() const {
  auto subtreeJson = this->toJson("");

  rapidjson::StringBuffer subtreeJsonBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(subtreeJsonBuffer);
  subtreeJson.Accept(writer);

  SubtreeHeader subtreeHeader;
  subtreeHeader.magic[0] = 's';
  subtreeHeader.magic[1] = 'u';
  subtreeHeader.magic[2] = 'b';
  subtreeHeader.magic[3] = 't';
  subtreeHeader.version = 1U;
  subtreeHeader.jsonByteLength = subtreeJsonBuffer.GetSize();
  subtreeHeader.binaryByteLength = this->buffers.size();

  std::vector<std::byte> buffer(size_t(
      sizeof(subtreeHeader) + subtreeHeader.jsonByteLength +
      subtreeHeader.binaryByteLength));
  std::memcpy(buffer.data(), &subtreeHeader, sizeof(subtreeHeader));
  std::memcpy(
      buffer.data() + sizeof(subtreeHeader),
      subtreeJsonBuffer.GetString(),
      size_t(subtreeHeader.jsonByteLength));
  std::memcpy(
      buffer.data() + sizeof(subtreeHeader) + subtreeHeader.jsonByteLength,
      this->buffers.data(),
      size_t(subtreeHeader.binaryByteLength));
  return buffer;
}

uint64_t getTotalNumberOfTilesForQuadtree(uint64_t maximumLevel) {
  switch (maximumLevel) {
  case 0:
    return 1;
  case 1:
    return 5;
  default:
    return static_cast<uint64_t>(
        (std::pow(4.0, static_cast<double>(maximumLevel)) - 1) /
        (static_cast<double>(maximumLevel) - 1));
  }
}

void markTileAvailable(
    const CesiumGeometry::QuadtreeTileID& tileID,
    std::span<std::byte> available) {
  // This function assumes that subtree tile ID is (0, 0, 0).
  // TileID must be within the subtree
  uint64_t numOfTilesFromRootToParentLevel =
      static_cast<uint64_t>(((1 << (2 * tileID.level)) - 1) / 3);
  uint64_t availabilityBitIndex =
      numOfTilesFromRootToParentLevel +
      libmorton::morton2D_64_encode(tileID.x, tileID.y);
  const size_t byteIndex = size_t(availabilityBitIndex / 8);
  const size_t bitIndex = size_t(availabilityBitIndex % 8);
  available[byteIndex] |= std::byte(1 << bitIndex);
}

void markSubtreeAvailable(
    const CesiumGeometry::QuadtreeTileID& tileID,
    std::span<std::byte> available) {
  uint64_t availabilityBitIndex =
      libmorton::morton2D_64_encode(tileID.x, tileID.y);
  const uint64_t byteIndex = availabilityBitIndex / 8;
  const uint64_t bitIndex = availabilityBitIndex % 8;
  available[(size_t)byteIndex] |= std::byte(1 << bitIndex);
}

} // namespace CesiumNativeTests
