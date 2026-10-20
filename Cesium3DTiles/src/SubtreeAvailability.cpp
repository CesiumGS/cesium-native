#include <Cesium3DTiles/ImplicitTileSubdivisionScheme.h>
#include <Cesium3DTiles/ImplicitTilingUtilities.h>
#include <Cesium3DTiles/SubtreeAvailability.h>
#include <Cesium3DTiles/SubtreeInterop.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGltf/Extension3dTilesSubtree.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumTilesetJson/Subtree.h>
#include <CesiumTilesetJsonReader/SubtreeFileReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Result.h>
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

using namespace CesiumTilesetJsonReader;
using namespace CesiumJsonReader;

namespace Cesium3DTiles {

/*static*/ std::optional<SubtreeAvailability> SubtreeAvailability::fromSubtree(
    ImplicitTileSubdivisionScheme subdivisionScheme,
    uint32_t levelsInSubtree,
    CesiumTilesetJson::Subtree&& subtree) noexcept {
  CesiumUtility::Result<CesiumGltf::Model> maybeModel =
      SubtreeInterop::convertToGltfModel(std::move(subtree));

  if (!maybeModel.value) {
    return std::nullopt;
  }

  return SubtreeAvailability::fromGltf(
      subdivisionScheme,
      levelsInSubtree,
      std::move(*maybeModel.value));
}

namespace {
std::optional<SubtreeAvailability::AvailabilityView> parseAvailabilityView(
    CesiumGltf::Model& model,
    const CesiumGltf::Availability& availability) {
  if (availability.constant) {
    return SubtreeAvailability::SubtreeConstantAvailability{
        *availability.constant ==
        CesiumTilesetJson::Availability::Constant::AVAILABLE};
  }

  CesiumGltf::BufferView* pBufferView =
      model.getSafe(&model.bufferViews, availability.bitstream);
  if (!pBufferView) {
    return std::nullopt;
  }

  CesiumGltf::Buffer* pBuffer =
      model.getSafe(&model.buffers, pBufferView->buffer);
  if (!pBuffer) {
    return std::nullopt;
  }

  std::vector<std::byte>& data = pBuffer->cesium.data;
  int64_t bufferSize =
      std::min(static_cast<int64_t>(data.size()), pBuffer->byteLength);
  if (pBufferView->byteLength >= 0 &&
      pBufferView->byteOffset + pBufferView->byteLength <= bufferSize) {
    return SubtreeAvailability::SubtreeBufferViewAvailability{
        std::span<std::byte>(
            data.data() + pBufferView->byteOffset,
            size_t(pBufferView->byteLength))};
  }

  return std::nullopt;
}
} // namespace

/*static*/ std::optional<SubtreeAvailability> SubtreeAvailability::fromGltf(
    ImplicitTileSubdivisionScheme subdivisionScheme,
    uint32_t levelsInSubtree,
    CesiumGltf::Model&& model) noexcept {
  const auto* pSubtreeExtension =
      model.getExtension<CesiumGltf::Extension3dTilesSubtree>();
  if (!pSubtreeExtension) {
    return std::nullopt;
  }
  std::optional<SubtreeAvailability::AvailabilityView> maybeTileAvailability =
      parseAvailabilityView(model, pSubtreeExtension->tileAvailability);
  if (!maybeTileAvailability)
    return std::nullopt;

  std::optional<SubtreeAvailability::AvailabilityView>
      maybeChildSubtreeAvailability = parseAvailabilityView(
          model,
          pSubtreeExtension->childSubtreeAvailability);
  if (!maybeChildSubtreeAvailability)
    return std::nullopt;

  std::optional<SubtreeAvailability::AvailabilityView> maybeContentAvailability;
  if (pSubtreeExtension->contentAvailability) {
    maybeContentAvailability =
        parseAvailabilityView(model, *pSubtreeExtension->contentAvailability);
  } else {
    // Fall back on an empty availability.
    maybeContentAvailability =
        SubtreeAvailability::SubtreeConstantAvailability{false};
  }

  if (!maybeContentAvailability) {
    // Only happens if something went wrong while parsing the
    // contentAvailability view.
    return std::nullopt;
  }

  return SubtreeAvailability(
      subdivisionScheme,
      levelsInSubtree,
      *maybeTileAvailability,
      *maybeChildSubtreeAvailability,
      *maybeContentAvailability,
      std::move(model));
}

/*static*/ std::optional<SubtreeAvailability> SubtreeAvailability::createEmpty(
    ImplicitTileSubdivisionScheme subdivisionScheme,
    uint32_t levelsInSubtree,
    bool setTilesAvailable) noexcept {
  CesiumTilesetJson::Subtree subtree;
  subtree.tileAvailability.constant =
      setTilesAvailable
          ? CesiumTilesetJson::Availability::Constant::AVAILABLE
          : CesiumTilesetJson::Availability::Constant::UNAVAILABLE;
  subtree.contentAvailability.emplace_back().constant =
      CesiumTilesetJson::Availability::Constant::UNAVAILABLE;
  subtree.childSubtreeAvailability.constant =
      CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

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
  if (subtreeUrl.ends_with(".gltf") || subtreeUrl.ends_with(".glb")) {
    auto pReader = std::make_shared<CesiumGltfReader::GltfReader>();
    return pReader
        ->loadGltf(asyncSystem, subtreeUrl, requestHeaders, pAssetAccessor)
        .thenInMainThread(
            [pLogger, subtreeUrl, subdivisionScheme, levelsInSubtree, pReader](
                CesiumGltfReader::GltfReaderResult&& result)
                -> std::optional<SubtreeAvailability> {
              if (!result.errors.empty()) {
                SPDLOG_LOGGER_ERROR(
                    pLogger,
                    "Errors while loading glTF subtree from {}:\n- {}",
                    subtreeUrl,
                    CesiumUtility::joinToString(result.errors, "\n- "));
              }
              if (!result.warnings.empty()) {
                SPDLOG_LOGGER_WARN(
                    pLogger,
                    "Warnings while loading glTF subtree from {}:\n- {}",
                    subtreeUrl,
                    CesiumUtility::joinToString(result.warnings, "\n- "));
              }

              if (!result.model) {
                return std::nullopt;
              }

              return SubtreeAvailability::fromGltf(
                  subdivisionScheme,
                  levelsInSubtree,
                  std::move(*result.model));
            });
  }

  auto pReader = std::make_shared<SubtreeFileReader>();
  return pReader->load(asyncSystem, pAssetAccessor, subtreeUrl, requestHeaders)
      .thenInMainThread(
          [pLogger, subtreeUrl, subdivisionScheme, levelsInSubtree, pReader](
              ReadJsonResult<CesiumTilesetJson::Subtree>&& subtree)
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
    AvailabilityView contentAvailability,
    CesiumGltf::Model&& model)
    : _powerOf2{subdivisionScheme == ImplicitTileSubdivisionScheme::Quadtree ? 2U : 3U},
      _levelsInSubtree{levelsInSubtree},
      _childCount{
          subdivisionScheme == ImplicitTileSubdivisionScheme::Quadtree ? 4U
                                                                       : 8U},
      _model{std::move(model)},
      _pSubtree{
          &this->_model.addExtension<CesiumGltf::Extension3dTilesSubtree>()},
      _tileAvailability{tileAvailability},
      _subtreeAvailability{subtreeAvailability},
      _contentAvailability{contentAvailability} {
  CESIUM_ASSERT(
      (this->_childCount == 4 || this->_childCount == 8) &&
      "SubtreeAvailability only supports quadtrees and octrees.");
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
      this->_pSubtree->tileAvailability,
      isAvailable);
}

bool SubtreeAvailability::isContentAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeId,
    const CesiumGeometry::QuadtreeTileID& tileId) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  return this->isContentAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx);
}

bool SubtreeAvailability::isContentAvailable(
    const CesiumGeometry::OctreeTileID& subtreeId,
    const CesiumGeometry::OctreeTileID& tileId) const noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  return this->isContentAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx);
}

bool SubtreeAvailability::isContentAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId) const noexcept {
  return isAvailable(
      relativeTileLevel,
      relativeTileMortonId,
      this->_contentAvailability);
}

void SubtreeAvailability::setContentAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeId,
    const CesiumGeometry::QuadtreeTileID& tileId,
    bool isAvailable) noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  this->setContentAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx,
      isAvailable);
}

void SubtreeAvailability::setContentAvailable(
    const CesiumGeometry::OctreeTileID& subtreeId,
    const CesiumGeometry::OctreeTileID& tileId,
    bool isAvailable) noexcept {
  uint64_t relativeTileMortonIdx =
      ImplicitTilingUtilities::computeRelativeMortonIndex(subtreeId, tileId);
  this->setContentAvailable(
      tileId.level - subtreeId.level,
      relativeTileMortonIdx,
      isAvailable);
}

void SubtreeAvailability::setContentAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId,
    bool isAvailable) noexcept {
  CESIUM_ASSERT(this->_pSubtree->contentAvailability);
  this->setAvailable(
      relativeTileLevel,
      relativeTileMortonId,
      this->_contentAvailability,
      *this->_pSubtree->contentAvailability,
      isAvailable);
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
          this->_model,
          numberOfTilesInNextLevel,
          this->_subtreeAvailability,
          this->_pSubtree->childSubtreeAvailability);
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
    CesiumGltf::Availability& availability,
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
          this->_model,
          numberOfTilesInSubtree,
          availabilityView,
          availability);
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

  const size_t byteIndex = size_t(availabilityBitIndex / 8);
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

  const size_t byteIndex = size_t(availabilityBitIndex / 8);
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

namespace {

void updateAvailabilityView(
    CesiumGltf::Model& model,
    SubtreeAvailability::AvailabilityView& availabilityView,
    CesiumGltf::Availability& availability) {
  if (!std::holds_alternative<
          SubtreeAvailability::SubtreeBufferViewAvailability>(
          availabilityView)) {
    return;
  }

  CESIUM_ASSERT(
      availability.bitstream >= 0 &&
      size_t(availability.bitstream) < model.bufferViews.size());

  const CesiumGltf::BufferView& bufferView =
      model.bufferViews[size_t(availability.bitstream)];

  CESIUM_ASSERT(
      bufferView.buffer >= 0 &&
      size_t(bufferView.buffer) < model.buffers.size());

  CesiumGltf::Buffer& buffer = model.buffers[size_t(bufferView.buffer)];

  std::span<std::byte> view(
      buffer.cesium.data.data() + bufferView.byteOffset,
      buffer.cesium.data.data() + bufferView.byteOffset +
          bufferView.byteLength);

  availabilityView = SubtreeAvailability::SubtreeBufferViewAvailability{view};
}

} // namespace

void SubtreeAvailability::updateAvailabilityViews() {
  updateAvailabilityView(
      this->_model,
      this->_tileAvailability,
      this->_pSubtree->tileAvailability);

  updateAvailabilityView(
      this->_model,
      this->_subtreeAvailability,
      this->_pSubtree->childSubtreeAvailability);

  CESIUM_ASSERT(this->_pSubtree->contentAvailability);
  updateAvailabilityView(
      this->_model,
      this->_contentAvailability,
      *this->_pSubtree->contentAvailability);
}

void SubtreeAvailability::convertConstantAvailabilityToBitstream(
    CesiumGltf::Model& model,
    uint64_t numberOfTiles,
    SubtreeAvailability::AvailabilityView& availabilityView,
    CesiumGltf::Availability& availability) {
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

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = 0;
  bufferView.byteLength = int64_t(numberOfBytes);

  CesiumGltf::Buffer& buffer =
      !model.buffers.empty() ? model.buffers[0] : model.buffers.emplace_back();

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

  availability.bitstream = int32_t(model.bufferViews.size() - 1);
  availability.constant.reset();

  // Fix spans that may have been invalidated when the buffer grew
  this->updateAvailabilityViews();
}

} // namespace Cesium3DTiles
