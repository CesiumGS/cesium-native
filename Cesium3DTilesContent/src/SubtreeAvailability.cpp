#include <Cesium3DTiles/ImplicitTiling.h>
#include <Cesium3DTiles/Subtree.h>
#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <Cesium3DTilesReader/SubtreeFileReader.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Uri.h>

#include <gsl/span>
#include <rapidjson/document.h>

#include <optional>
#include <string>

using namespace Cesium3DTiles;
using namespace Cesium3DTilesReader;
using namespace CesiumJsonReader;

namespace Cesium3DTilesContent {

namespace {

std::optional<AvailabilityView> parseAvailabilityView(
    const Cesium3DTiles::Availability& availability,
    const std::vector<Cesium3DTiles::Buffer>& buffers,
    const std::vector<Cesium3DTiles::BufferView>& bufferViews) {
  if (availability.constant) {
    return SubtreeConstantAvailability{*availability.constant == 1};
  }

  int64_t bufferViewIndex = -1;

  if (availability.bitstream) {
    bufferViewIndex = *availability.bitstream;
  } else {
    // old version uses bufferView property instead of bitstream. Same semantic
    // either way
    auto bufferViewIt = availability.unknownProperties.find("bufferView");
    if (bufferViewIt != availability.unknownProperties.end()) {
      bufferViewIndex =
          bufferViewIt->second.getSafeNumberOrDefault<int64_t>(-1);
    }
  }

  if (bufferViewIndex >= 0 &&
      bufferViewIndex < static_cast<int64_t>(bufferViews.size())) {
    const Cesium3DTiles::BufferView& bufferView = bufferViews[bufferViewIndex];

    if (bufferView.buffer >= 0 &&
        bufferView.buffer < static_cast<int64_t>(buffers.size())) {
      const Cesium3DTiles::Buffer& buffer = buffers[bufferView.buffer];
      const std::vector<std::byte>& data = buffer.cesium.data;
      int64_t bufferSize =
          std::min(static_cast<int64_t>(data.size()), buffer.byteLength);
      if (bufferView.byteOffset + bufferView.byteLength <= bufferSize) {
        return SubtreeBufferViewAvailability{gsl::span<const std::byte>(
            data.data() + bufferView.byteOffset,
            bufferView.byteLength)};
      }
    }
  }

  return std::nullopt;
}

} // namespace

/*static*/ std::optional<SubtreeAvailability> SubtreeAvailability::fromSubtree(
    uint32_t powerOfTwo,
    Cesium3DTiles::Subtree&& subtree) noexcept {
  std::optional<AvailabilityView> maybeTileAvailability = parseAvailabilityView(
      subtree.tileAvailability,
      subtree.buffers,
      subtree.bufferViews);
  if (!maybeTileAvailability)
    return std::nullopt;

  std::optional<AvailabilityView> maybeChildSubtreeAvailability =
      parseAvailabilityView(
          subtree.childSubtreeAvailability,
          subtree.buffers,
          subtree.bufferViews);
  if (!maybeChildSubtreeAvailability)
    return std::nullopt;

  // At least one element is required in contentAvailability.
  if (subtree.contentAvailability.empty())
    return std::nullopt;

  std::vector<AvailabilityView> contentAvailability;
  contentAvailability.reserve(subtree.contentAvailability.size());

  for (const auto& availabilityDesc : subtree.contentAvailability) {
    auto maybeAvailability = parseAvailabilityView(
        availabilityDesc,
        subtree.buffers,
        subtree.bufferViews);
    if (maybeAvailability) {
      contentAvailability.emplace_back(std::move(*maybeAvailability));
    }
  }

  return SubtreeAvailability(
      powerOfTwo,
      *maybeTileAvailability,
      *maybeChildSubtreeAvailability,
      std::move(contentAvailability),
      std::move(subtree));
}

/*static*/ CesiumAsync::Future<std::optional<SubtreeAvailability>>
SubtreeAvailability::loadSubtree(
    uint32_t powerOf2,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& subtreeUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  SubtreeFileReader reader;
  return reader.load(asyncSystem, pAssetAccessor, subtreeUrl, requestHeaders)
      .thenInMainThread(
          [pLogger, subtreeUrl, powerOf2](ReadJsonResult<Subtree>&& subtree)
              -> std::optional<SubtreeAvailability> {
            if (!subtree.value) {
              if (!subtree.errors.empty()) {
                SPDLOG_LOGGER_ERROR(
                    pLogger,
                    "Errors while loading subtree from {}:\n- {}",
                    subtreeUrl,
                    CesiumUtility::joinToString(subtree.errors, "\n- "));
              }
              if (!subtree.warnings.empty()) {
                SPDLOG_LOGGER_WARN(
                    pLogger,
                    "Warnings while loading subtree from {}:\n- {}",
                    subtreeUrl,
                    CesiumUtility::joinToString(subtree.warnings, "\n- "));
              }
              return std::nullopt;
            }

            return SubtreeAvailability::fromSubtree(
                powerOf2,
                std::move(*subtree.value));
          });
}

SubtreeAvailability::SubtreeAvailability(
    uint32_t powerOf2,
    AvailabilityView tileAvailability,
    AvailabilityView subtreeAvailability,
    std::vector<AvailabilityView>&& contentAvailability,
    Cesium3DTiles::Subtree&& subtree)
    : _subtree(std::move(subtree)),
      _childCount{1U << powerOf2},
      _powerOf2{powerOf2},
      _tileAvailability{tileAvailability},
      _subtreeAvailability{subtreeAvailability},
      _contentAvailability{std::move(contentAvailability)} {
  assert(
      (this->_childCount == 4 || this->_childCount == 8) &&
      "Only support quadtree and octree");
}

bool SubtreeAvailability::isTileAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeID,
    const CesiumGeometry::QuadtreeTileID& tileID) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeID, tileID);
  return this->isTileAvailable(
      tileID.level - subtreeID.level,
      relativeTileMortonIdx);
}

bool SubtreeAvailability::isTileAvailable(
    const CesiumGeometry::OctreeTileID& subtreeID,
    const CesiumGeometry::OctreeTileID& tileID) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeID, tileID);
  return this->isTileAvailable(
      tileID.level - subtreeID.level,
      relativeTileMortonIdx);
}

bool SubtreeAvailability::isTileAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId) const noexcept {
  return isAvailable(
      relativeTileLevel,
      relativeTileMortonId,
      this->_tileAvailability);
}

bool SubtreeAvailability::isContentAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeID,
    const CesiumGeometry::QuadtreeTileID& tileID,
    uint64_t contentId) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeID, tileID);
  return this->isContentAvailable(
      tileID.level - subtreeID.level,
      relativeTileMortonIdx,
      contentId);
}

bool SubtreeAvailability::isContentAvailable(
    const CesiumGeometry::OctreeTileID& subtreeID,
    const CesiumGeometry::OctreeTileID& tileID,
    uint64_t contentId) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeID, tileID);
  return this->isContentAvailable(
      tileID.level - subtreeID.level,
      relativeTileMortonIdx,
      contentId);
}

bool SubtreeAvailability::isContentAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId,
    uint64_t contentId) const noexcept {
  return isAvailable(
      relativeTileLevel,
      relativeTileMortonId,
      this->_contentAvailability[contentId]);
}

bool SubtreeAvailability::isSubtreeAvailable(
    uint64_t relativeSubtreeMortonId) const noexcept {
  const SubtreeConstantAvailability* constantAvailability =
      std::get_if<SubtreeConstantAvailability>(&this->_subtreeAvailability);
  if (constantAvailability) {
    return constantAvailability->constant;
  }

  return isAvailableUsingBufferView(
      0,
      relativeSubtreeMortonId,
      this->_subtreeAvailability);
}

bool SubtreeAvailability::isAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId,
    const AvailabilityView& availabilityView) const noexcept {
  uint64_t numOfTilesInLevel = uint64_t(1)
                               << (this->_powerOf2 * relativeTileLevel);
  if (relativeTileMortonId >= numOfTilesInLevel) {
    return false;
  }

  const SubtreeConstantAvailability* constantAvailability =
      std::get_if<SubtreeConstantAvailability>(&availabilityView);
  if (constantAvailability) {
    return constantAvailability->constant;
  }

  uint64_t numOfTilesFromRootToParentLevel =
      (numOfTilesInLevel - 1U) / (this->_childCount - 1U);

  return isAvailableUsingBufferView(
      numOfTilesFromRootToParentLevel,
      relativeTileMortonId,
      availabilityView);
}

bool SubtreeAvailability::isAvailableUsingBufferView(
    uint64_t numOfTilesFromRootToParentLevel,
    uint64_t relativeTileMortonId,
    const AvailabilityView& availabilityView) const noexcept {

  uint64_t availabilityBitIndex =
      numOfTilesFromRootToParentLevel + relativeTileMortonId;

  const SubtreeBufferViewAvailability* bufferViewAvailability =
      std::get_if<SubtreeBufferViewAvailability>(&availabilityView);

  const uint64_t byteIndex = availabilityBitIndex / 8;
  if (byteIndex >= bufferViewAvailability->view.size()) {
    return false;
  }

  const uint64_t bitIndex = availabilityBitIndex % 8;
  const int bitValue =
      static_cast<int>(bufferViewAvailability->view[byteIndex] >> bitIndex) & 1;

  return bitValue == 1;
}
} // namespace Cesium3DTilesContent
