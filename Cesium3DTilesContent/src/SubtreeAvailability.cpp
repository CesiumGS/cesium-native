#include <Cesium3DTiles/Availability.h>
#include <Cesium3DTiles/Buffer.h>
#include <Cesium3DTiles/BufferView.h>
#include <Cesium3DTiles/Subtree.h>
#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <Cesium3DTilesReader/SubtreeFileReader.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/joinToString.h>

#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace Cesium3DTiles;
using namespace Cesium3DTilesReader;
using namespace CesiumJsonReader;

namespace Cesium3DTilesContent {

namespace {

std::optional<SubtreeAvailability::AvailabilityView> parseAvailabilityView(
    const Cesium3DTiles::Availability& availability,
    std::vector<Cesium3DTiles::Buffer>& buffers,
    std::vector<Cesium3DTiles::BufferView>& bufferViews) {
  if (availability.constant) {
    return SubtreeAvailability::SubtreeConstantAvailability{
        *availability.constant ==
        Cesium3DTiles::Availability::Constant::AVAILABLE};
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
    const Cesium3DTiles::BufferView& bufferView =
        bufferViews[size_t(bufferViewIndex)];

    if (bufferView.buffer >= 0 &&
        bufferView.buffer < static_cast<int64_t>(buffers.size())) {
      Cesium3DTiles::Buffer& buffer = buffers[size_t(bufferView.buffer)];
      std::vector<std::byte>& data = buffer.cesium.data;
      int64_t bufferSize =
          std::min(static_cast<int64_t>(data.size()), buffer.byteLength);
      if (bufferView.byteLength >= 0 &&
          bufferView.byteOffset + bufferView.byteLength <= bufferSize) {
        return SubtreeAvailability::SubtreeBufferViewAvailability{
            std::span<std::byte>(
                data.data() + bufferView.byteOffset,
                size_t(bufferView.byteLength))};
      }
    }
  }

  return std::nullopt;
}

} // namespace

/*static*/ std::optional<SubtreeAvailability> SubtreeAvailability::fromSubtree(
    ImplicitTileSubdivisionScheme subdivisionScheme,
    uint32_t levelsInSubtree,
    Cesium3DTiles::Subtree&& subtree) noexcept {
  std::optional<SubtreeAvailability::AvailabilityView> maybeTileAvailability =
      parseAvailabilityView(
          subtree.tileAvailability,
          subtree.buffers,
          subtree.bufferViews);
  if (!maybeTileAvailability)
    return std::nullopt;

  std::optional<SubtreeAvailability::AvailabilityView>
      maybeChildSubtreeAvailability = parseAvailabilityView(
          subtree.childSubtreeAvailability,
          subtree.buffers,
          subtree.bufferViews);
  if (!maybeChildSubtreeAvailability)
    return std::nullopt;

  // At least one element is required in contentAvailability.
  if (subtree.contentAvailability.empty())
    return std::nullopt;

  std::vector<SubtreeAvailability::AvailabilityView> contentAvailability;
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
      subdivisionScheme,
      levelsInSubtree,
      *maybeTileAvailability,
      *maybeChildSubtreeAvailability,
      std::move(contentAvailability),
      std::move(subtree));
}

/*static*/ std::optional<SubtreeAvailability> SubtreeAvailability::createEmpty(
    ImplicitTileSubdivisionScheme subdivisionScheme,
    uint32_t levelsInSubtree) noexcept {
  Subtree subtree;
  subtree.tileAvailability.constant =
      Cesium3DTiles::Availability::Constant::AVAILABLE;
  subtree.contentAvailability.emplace_back().constant =
      Cesium3DTiles::Availability::Constant::UNAVAILABLE;
  subtree.childSubtreeAvailability.constant =
      Cesium3DTiles::Availability::Constant::UNAVAILABLE;

  return SubtreeAvailability::fromSubtree(
      subdivisionScheme,
      levelsInSubtree,
      std::move(subtree));
}

/*static*/ CesiumAsync::Future<std::optional<SubtreeAvailability>>
SubtreeAvailability::loadSubtree(
    ImplicitTileSubdivisionScheme subdivisionScheme,
    uint32_t levelsInSubtree,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& subtreeUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  auto pReader = std::make_shared<SubtreeFileReader>();
  return pReader->load(asyncSystem, pAssetAccessor, subtreeUrl, requestHeaders)
      .thenInMainThread(
          [pLogger, subtreeUrl, subdivisionScheme, levelsInSubtree, pReader](
              ReadJsonResult<Subtree>&& subtree)
              -> std::optional<SubtreeAvailability> {
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

            if (!subtree.value) {
              return std::nullopt;
            }

            return SubtreeAvailability::fromSubtree(
                subdivisionScheme,
                levelsInSubtree,
                std::move(*subtree.value));
          });
}

SubtreeAvailability::SubtreeAvailability(
    ImplicitTileSubdivisionScheme subdivisionScheme,
    uint32_t levelsInSubtree,
    AvailabilityView tileAvailability,
    AvailabilityView subtreeAvailability,
    std::vector<AvailabilityView>&& contentAvailability,
    Cesium3DTiles::Subtree&& subtree)
    : _powerOf2{subdivisionScheme == ImplicitTileSubdivisionScheme::Quadtree ? 2U : 3U},
      _levelsInSubtree{levelsInSubtree},
      _subtree{std::move(subtree)},
      _childCount{
          subdivisionScheme == ImplicitTileSubdivisionScheme::Quadtree ? 4U
                                                                       : 8U},
      _tileAvailability{tileAvailability},
      _subtreeAvailability{subtreeAvailability},
      _contentAvailability{std::move(contentAvailability)} {
  CESIUM_ASSERT(
      (this->_childCount == 4 || this->_childCount == 8) &&
      "Only support quadtree and octree");
}

bool SubtreeAvailability::isTileAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeId,
    const CesiumGeometry::QuadtreeTileID& tileId) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  return this->isTileAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx);
}

bool SubtreeAvailability::isTileAvailable(
    const CesiumGeometry::OctreeTileID& subtreeId,
    const CesiumGeometry::OctreeTileID& tileId) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  return this->isTileAvailable(
      tileId.level - subtreeId.level,
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

void SubtreeAvailability::setTileAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeId,
    const CesiumGeometry::QuadtreeTileID& tileId,
    bool isAvailable) noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  this->setTileAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx,
      isAvailable);
}

void SubtreeAvailability::setTileAvailable(
    const CesiumGeometry::OctreeTileID& subtreeId,
    const CesiumGeometry::OctreeTileID& tileId,
    bool isAvailable) noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  this->setTileAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx,
      isAvailable);
}

void SubtreeAvailability::setTileAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId,
    bool isAvailable) noexcept {
  this->setAvailable(
      relativeTileLevel,
      relativeTileMortonId,
      this->_tileAvailability,
      isAvailable);
}

bool SubtreeAvailability::isContentAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeId,
    const CesiumGeometry::QuadtreeTileID& tileId,
    uint64_t contentId) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  return this->isContentAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx,
      contentId);
}

bool SubtreeAvailability::isContentAvailable(
    const CesiumGeometry::OctreeTileID& subtreeId,
    const CesiumGeometry::OctreeTileID& tileId,
    uint64_t contentId) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  return this->isContentAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx,
      contentId);
}

bool SubtreeAvailability::isContentAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId,
    uint64_t contentId) const noexcept {
  if (contentId >= this->_contentAvailability.size())
    return false;
  return isAvailable(
      relativeTileLevel,
      relativeTileMortonId,
      this->_contentAvailability[contentId]);
}

void SubtreeAvailability::setContentAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeId,
    const CesiumGeometry::QuadtreeTileID& tileId,
    uint64_t contentId,
    bool isAvailable) noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  this->setContentAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx,
      contentId,
      isAvailable);
}

void SubtreeAvailability::setContentAvailable(
    const CesiumGeometry::OctreeTileID& subtreeId,
    const CesiumGeometry::OctreeTileID& tileId,
    uint64_t contentId,
    bool isAvailable) noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  this->setContentAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx,
      contentId,
      isAvailable);
}

void SubtreeAvailability::setContentAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId,
    uint64_t contentId,
    bool isAvailable) noexcept {
  if (contentId < this->_contentAvailability.size()) {
    this->setAvailable(
        relativeTileLevel,
        relativeTileMortonId,
        this->_contentAvailability[contentId],
        isAvailable);
  }
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

bool SubtreeAvailability::isSubtreeAvailable(
    const CesiumGeometry::QuadtreeTileID& thisSubtreeID,
    const CesiumGeometry::QuadtreeTileID& checkSubtreeID) const noexcept {
  return isSubtreeAvailable(ImplicitTilingUtilities::computeRelativeMortonIndex(
      thisSubtreeID,
      checkSubtreeID));
}

bool SubtreeAvailability::isSubtreeAvailable(
    const CesiumGeometry::OctreeTileID& thisSubtreeID,
    const CesiumGeometry::OctreeTileID& checkSubtreeID) const noexcept {
  return isSubtreeAvailable(ImplicitTilingUtilities::computeRelativeMortonIndex(
      thisSubtreeID,
      checkSubtreeID));
}

namespace {

void convertConstantAvailabilityToBitstream(
    Subtree& subtree,
    uint64_t numberOfTiles,
    SubtreeAvailability::AvailabilityView& availabilityView) {
  const SubtreeAvailability::SubtreeConstantAvailability*
      pConstantAvailability =
          std::get_if<SubtreeAvailability::SubtreeConstantAvailability>(
              &availabilityView);
  if (!pConstantAvailability)
    return;

  bool oldValue = pConstantAvailability->constant;

  uint64_t numberOfBytes = numberOfTiles / 8;
  if (numberOfBytes * 8 < numberOfTiles)
    ++numberOfBytes;

  BufferView& bufferView = subtree.bufferViews.emplace_back();
  bufferView.buffer = 0;
  bufferView.byteLength = int64_t(numberOfBytes);

  Buffer& buffer = !subtree.buffers.empty() ? subtree.buffers[0]
                                            : subtree.buffers.emplace_back();

  int64_t start = buffer.byteLength;

  // Align the new bufferView to a multiple of 8 bytes, as required by the spec.
  int64_t paddingRemainder = start % 8;
  if (paddingRemainder > 0) {
    start += 8 - paddingRemainder;
  }

  int64_t end = start + int64_t(numberOfBytes);

  bufferView.byteOffset = start;
  buffer.byteLength = end;

  buffer.cesium.data.resize(
      size_t(buffer.byteLength),
      oldValue ? std::byte(0xFF) : std::byte(0x00));

  std::span<std::byte> view(
      buffer.cesium.data.data() + start,
      buffer.cesium.data.data() + end);
  availabilityView = SubtreeAvailability::SubtreeBufferViewAvailability{view};
}

} // namespace

void SubtreeAvailability::setSubtreeAvailable(
    uint64_t relativeSubtreeMortonId,
    bool isAvailable) noexcept {
  const SubtreeConstantAvailability* pConstantAvailability =
      std::get_if<SubtreeConstantAvailability>(&this->_subtreeAvailability);
  if (pConstantAvailability) {
    if (pConstantAvailability->constant == isAvailable) {
      // New state matches the constant, so there is nothing to do.
      return;
    } else {
      uint64_t numberOfTilesInNextLevel =
          uint64_t(1) << (this->_powerOf2 * this->_levelsInSubtree);

      convertConstantAvailabilityToBitstream(
          this->_subtree,
          numberOfTilesInNextLevel,
          this->_subtreeAvailability);
    }
  }

  setAvailableUsingBufferView(
      0,
      relativeSubtreeMortonId,
      this->_subtreeAvailability,
      isAvailable);
}

void SubtreeAvailability::setSubtreeAvailable(
    const CesiumGeometry::QuadtreeTileID& thisSubtreeID,
    const CesiumGeometry::QuadtreeTileID& setSubtreeID,
    bool isAvailable) noexcept {
  this->setSubtreeAvailable(
      ImplicitTilingUtilities::computeRelativeMortonIndex(
          thisSubtreeID,
          setSubtreeID),
      isAvailable);
}

void SubtreeAvailability::setSubtreeAvailable(
    const CesiumGeometry::OctreeTileID& thisSubtreeID,
    const CesiumGeometry::OctreeTileID& setSubtreeID,
    bool isAvailable) noexcept {
  this->setSubtreeAvailable(
      ImplicitTilingUtilities::computeRelativeMortonIndex(
          thisSubtreeID,
          setSubtreeID),
      isAvailable);
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

void SubtreeAvailability::setAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId,
    AvailabilityView& availabilityView,
    bool isAvailable) noexcept {
  const SubtreeConstantAvailability* pConstantAvailability =
      std::get_if<SubtreeConstantAvailability>(&availabilityView);
  if (pConstantAvailability) {
    if (pConstantAvailability->constant == isAvailable) {
      // New state matches the constant, so there is nothing to do.
      return;
    } else {
      uint64_t numberOfTilesInNextLevel =
          uint64_t(1) << (this->_powerOf2 * this->_levelsInSubtree);
      uint64_t numberOfTilesInSubtree =
          (numberOfTilesInNextLevel - 1U) / (this->_childCount - 1U);

      convertConstantAvailabilityToBitstream(
          this->_subtree,
          numberOfTilesInSubtree,
          availabilityView);
    }
  }

  // At this point we're definitely working with a bitstream (not a constant).
  uint64_t numOfTilesInLevel = uint64_t(1)
                               << (this->_powerOf2 * relativeTileLevel);
  if (relativeTileMortonId >= numOfTilesInLevel) {
    // Attempting to set an invalid level. Assert, but otherwise ignore it.
    CESIUM_ASSERT(false);
    return;
  }

  uint64_t numOfTilesFromRootToParentLevel =
      (numOfTilesInLevel - 1U) / (this->_childCount - 1U);

  return setAvailableUsingBufferView(
      numOfTilesFromRootToParentLevel,
      relativeTileMortonId,
      availabilityView,
      isAvailable);
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

void SubtreeAvailability::setAvailableUsingBufferView(
    uint64_t numOfTilesFromRootToParentLevel,
    uint64_t relativeTileMortonId,
    AvailabilityView& availabilityView,
    bool isAvailable) noexcept {
  uint64_t availabilityBitIndex =
      numOfTilesFromRootToParentLevel + relativeTileMortonId;

  const SubtreeBufferViewAvailability* pBufferViewAvailability =
      std::get_if<SubtreeBufferViewAvailability>(&availabilityView);

  const uint64_t byteIndex = availabilityBitIndex / 8;
  if (byteIndex >= pBufferViewAvailability->view.size()) {
    // Attempting to set an invalid tile. Assert, but otherwise ignore it.
    CESIUM_ASSERT(false);
    return;
  }

  const uint64_t bitIndex = availabilityBitIndex % 8;

  if (isAvailable) {
    pBufferViewAvailability->view[byteIndex] |= std::byte(1) << bitIndex;
  } else {
    pBufferViewAvailability->view[byteIndex] &= ~(std::byte(1) << bitIndex);
  }
}

} // namespace Cesium3DTilesContent
