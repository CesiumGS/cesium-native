#include <Cesium3DTiles/SubtreeInterop.h>
#include <CesiumGltf/BufferCesium.h>
#include <CesiumGltf/Extension3dTilesSubtree.h>
#include <CesiumGltf/Model.h>
#include <CesiumTilesetJson/Subtree.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <bit>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

using namespace CesiumUtility;

namespace Cesium3DTiles {

namespace {
inline CesiumGltf::BufferCesium
convertBufferCesium(CesiumTilesetJson::BufferCesium&& buffer) {
  return CesiumGltf::BufferCesium{std::move(buffer.data)};
}

inline CesiumTilesetJson::BufferCesium
convertBufferCesium(CesiumGltf::BufferCesium&& buffer) {
  return CesiumTilesetJson::BufferCesium{std::move(buffer.data)};
}

inline void moveCommonProperties(
    CesiumUtility::ExtensibleObject& destination,
    CesiumUtility::ExtensibleObject& source) {
  destination.extensions = std::move(source.extensions);
  destination.extras = std::move(source.extras);
  destination.unknownProperties = std::move(source.unknownProperties);
}
} // namespace

CesiumGltf::Buffer
SubtreeInterop::convertBuffer(CesiumTilesetJson::Buffer&& buffer) {
  CesiumGltf::Buffer result;
  moveCommonProperties(result, buffer);
  if (buffer.name) {
    result.name = std::move(*buffer.name);
  }
  result.uri = buffer.uri;
  result.byteLength = buffer.byteLength;
  result.cesium = convertBufferCesium(std::move(buffer.cesium));

  return result;
}

CesiumUtility::Result<CesiumTilesetJson::Buffer>
SubtreeInterop::convertBuffer(CesiumGltf::Buffer&& buffer) {
  CesiumTilesetJson::Buffer result;
  moveCommonProperties(result, buffer);
  if (buffer.name.size() > 0) {
    result.name.emplace(std::move(buffer.name));
  }
  result.uri = buffer.uri;
  result.byteLength = buffer.byteLength;
  result.cesium = convertBufferCesium(std::move(buffer.cesium));

  bool hasData = result.uri.has_value() || !result.cesium.data.empty();

  if (!hasData && buffer.chunk >= 0) {
    CesiumUtility::ErrorList errors;
    errors.emplaceError("Input glTF 2.1 buffer specifies a `chunk` index that "
                        "has not been resolved and is thus missing data.");
    return Result<CesiumTilesetJson::Buffer>(std::move(errors));
  }

  return Result(std::move(result));
}

CesiumUtility::Result<CesiumGltf::BufferView>
SubtreeInterop::convertBufferView(CesiumTilesetJson::BufferView&& bufferView) {
  CesiumGltf::BufferView result;
  moveCommonProperties(result, bufferView);

  if (bufferView.name) {
    result.name = std::move(*bufferView.name);
  }

  if (bufferView.buffer > std::numeric_limits<int32_t>::max()) {
    CesiumUtility::ErrorList errors;
    errors.emplaceError(
        "Input 3D Tiles 1.1 buffer view specifies a buffer index that is too "
        "large to represent in cesium-native's glTF implementation.");
    return Result<CesiumGltf::BufferView>(std::move(errors));
  }

  result.buffer = static_cast<int32_t>(bufferView.buffer);
  result.byteLength = bufferView.byteLength;
  result.byteOffset = bufferView.byteOffset;

  return result;
}

CesiumUtility::Result<CesiumTilesetJson::BufferView>
SubtreeInterop::convertBufferView(CesiumGltf::BufferView&& bufferView) {
  CesiumTilesetJson::BufferView result;
  moveCommonProperties(result, bufferView);

  if (bufferView.name.size() > 0) {
    result.name.emplace(std::move(bufferView.name));
  }

  result.buffer = bufferView.buffer;
  result.byteLength = bufferView.byteLength;
  result.byteOffset = bufferView.byteOffset;

  CesiumUtility::ErrorList errors;
  if (bufferView.byteStride) {
    errors.emplaceWarning(
        "The `byteStride` property in glTF buffer views has no equivalent in "
        "3D Tiles 1.1. The value will be stored in the buffer view's "
        "`unknownProperties`.");
    result.unknownProperties.emplace("byteStride", *bufferView.byteStride);
  }

  return Result(std::move(result), std::move(errors));
}

CesiumGltf::Availability SubtreeInterop::convertAvailability(
    CesiumTilesetJson::Availability&& availability) {
  CesiumGltf::Availability result;
  moveCommonProperties(result, availability);

  if (availability.bitstream) {
    result.bitstream = static_cast<int32_t>(*availability.bitstream);
  } else {
    // An older version of the subtree format uses "bufferView" instead of
    // "bitstream". Same semantic either way
    auto bufferViewIt = result.unknownProperties.find("bufferView");
    if (bufferViewIt != result.unknownProperties.end()) {
      result.bitstream =
          bufferViewIt->second.getSafeNumberOrDefault<int32_t>(-1);
      result.unknownProperties.erase(bufferViewIt);
    }
  }
  result.availableCount = availability.availableCount;

  CESIUM_ASSERT(
      CesiumGltf::Availability::Constant::UNAVAILABLE ==
      CesiumTilesetJson::Availability::Constant::UNAVAILABLE);
  CESIUM_ASSERT(
      CesiumGltf::Availability::Constant::AVAILABLE ==
      CesiumTilesetJson::Availability::Constant::AVAILABLE);
  result.constant = availability.constant;

  return result;
}

CesiumTilesetJson::Availability
SubtreeInterop::convertAvailability(CesiumGltf::Availability&& availability) {
  CesiumTilesetJson::Availability result;
  moveCommonProperties(result, availability);

  if (availability.bitstream >= 0) {
    result.bitstream = availability.bitstream;
  }
  result.availableCount = availability.availableCount;

  CESIUM_ASSERT(
      CesiumGltf::Availability::Constant::UNAVAILABLE ==
      CesiumTilesetJson::Availability::Constant::UNAVAILABLE);
  CESIUM_ASSERT(
      CesiumGltf::Availability::Constant::AVAILABLE ==
      CesiumTilesetJson::Availability::Constant::AVAILABLE);
  result.constant = availability.constant;
  return result;
}

namespace {
/**
 * @brief Merges all content availabilities in a @ref
 * CesiumTilesetJson::Subtree into one @ref CesiumTilesetJson::Availability.
 * This may result in the allocation of a new buffer on the subtree.
 *
 * @returns Any errors that emerged during the operation.
 */
CesiumUtility::ErrorList
mergeContentAvailability(CesiumTilesetJson::Subtree& subtree) {
  CesiumUtility::ErrorList errors;
  if (subtree.contentAvailability.size() <= 1) {
    return errors;
  }

  bool allAvailable = false;
  std::vector<std::byte> bitstreamData;

  for (CesiumTilesetJson::Availability& availability :
       subtree.contentAvailability) {
    if (availability.constant) {
      if (*availability.constant ==
          CesiumGltf::Availability::Constant::AVAILABLE) {
        allAvailable = true;
        break;
      } else {
        continue;
      }
    }

    if (!availability.bitstream) {
      continue;
    }

    int64_t bitstream = *availability.bitstream;
    if (bitstream < 0 || size_t(bitstream) >= subtree.bufferViews.size()) {
      errors.emplaceError(
          "Invalid bitstream " + std::to_string(bitstream) +
          " in subtree content availability.");
      return errors;
    }

    CesiumTilesetJson::BufferView& bufferView =
        subtree.bufferViews[size_t(bitstream)];
    if (bufferView.buffer < 0 ||
        size_t(bufferView.buffer) >= subtree.buffers.size()) {
      errors.emplaceError(
          "Invalid buffer index " + std::to_string(bufferView.buffer) +
          " for bitstream " + std::to_string(bitstream) +
          " in subtree content availability.");
      return errors;
    }

    CesiumTilesetJson::Buffer& buffer =
        subtree.buffers[size_t(bufferView.buffer)];
    if (bufferView.byteLength > buffer.byteLength - bufferView.byteOffset) {
      errors.emplaceError("Buffer view for availability bitstream extends "
                          "beyond buffer byteLength.");
      return errors;
    }

    if (buffer.cesium.data.empty()) {
      errors.emplaceError(
          "Buffer used for availability bitstream " +
          std::to_string(bitstream) +
          " has unresolved buffer content and cannot be converted to glTF "
          "availability.");
      return errors;
    }

    if (bitstreamData.empty()) {
      // Copy data directly from the first availability entry.
      bitstreamData = buffer.cesium.data;
      continue;
    }

    if (size_t(bufferView.byteLength) != bitstreamData.size()) {
      errors.emplaceError("Found differing sizes of availability bitstreams in "
                          "the given subtree.");
      return errors;
    }

    std::span<std::byte> availabilityData(
        buffer.cesium.data.data() + bufferView.byteOffset,
        size_t(bufferView.byteLength));
    for (size_t i = 0; i < bitstreamData.size(); i++) {
      bitstreamData[i] |= availabilityData[i];
    }
  }

  subtree.contentAvailability.resize(1);

  CesiumTilesetJson::Availability& availability =
      subtree.contentAvailability[0];
  availability.constant.reset();
  availability.bitstream.reset();
  availability.availableCount.reset();

  if (allAvailable) {
    availability.constant =
        CesiumTilesetJson::Availability::Constant::AVAILABLE;
  } else if (bitstreamData.size() > 0) {
    int64_t availableCount = 0;
    for (const std::byte& byte : bitstreamData) {
      availableCount += std::popcount(uint8_t(byte));
    }

    CesiumTilesetJson::Buffer& buffer = subtree.buffers.emplace_back();
    buffer.cesium.data = std::move(bitstreamData);
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    CesiumTilesetJson::BufferView& bufferView =
        subtree.bufferViews.emplace_back();
    bufferView.buffer = static_cast<int64_t>(subtree.buffers.size() - 1);
    bufferView.byteLength = buffer.byteLength;

    availability.bitstream =
        static_cast<int64_t>(subtree.bufferViews.size() - 1);
    availability.availableCount = availableCount;
  } else {
    availability.constant =
        CesiumTilesetJson::Availability::Constant::UNAVAILABLE;
  }

  return errors;
}

} // namespace

CesiumUtility::Result<CesiumGltf::Model>
SubtreeInterop::convertToGltfModel(CesiumTilesetJson::Subtree&& subtree) {
  CesiumGltf::Model model;
  moveCommonProperties(model, subtree);

  // Content availability requires more effort to convert due to multiple
  // contents in 3D Tiles 1.1. Merge it into a single availability object before
  // doing anything else in the subtree.
  CesiumUtility::ErrorList errors = mergeContentAvailability(subtree);
  if (errors.hasErrors()) {
    return Result<CesiumGltf::Model>(std::move(errors));
  }

  const size_t bufferCount = subtree.buffers.size();
  model.buffers.resize(bufferCount);
  for (size_t i = 0; i < bufferCount; i++) {
    CesiumGltf::Buffer buffer = convertBuffer(std::move(subtree.buffers[i]));
    model.buffers[i] = std::move(buffer);
  }

  const size_t bufferViewCount = subtree.bufferViews.size();
  model.bufferViews.resize(bufferViewCount);
  for (size_t i = 0; i < bufferViewCount; i++) {
    CesiumUtility::Result<CesiumGltf::BufferView> bufferViewResult =
        convertBufferView(std::move(subtree.bufferViews[i]));
    if (bufferViewResult.value) {
      model.bufferViews[i] = std::move(*bufferViewResult.value);
    }
    errors.merge(std::move(bufferViewResult.errors));
  }

  auto& subtreeExtension =
      model.addExtension<CesiumGltf::Extension3dTilesSubtree>();
  model.addExtensionRequired(
      CesiumGltf::Extension3dTilesSubtree::ExtensionName);
  model.addExtensionUsed(CesiumGltf::Extension3dTilesSubtree::ExtensionName);

  subtreeExtension.tileAvailability =
      convertAvailability(std::move(subtree.tileAvailability));
  subtreeExtension.childSubtreeAvailability =
      convertAvailability(std::move(subtree.childSubtreeAvailability));

  if (subtree.contentAvailability.size() > 0) {
    CESIUM_ASSERT(subtree.contentAvailability.size() == 1);
    subtreeExtension.contentAvailability =
        convertAvailability(std::move(subtree.contentAvailability[0]));
  }

  return Result(std::move(model), std::move(errors));
}

CesiumUtility::Result<CesiumTilesetJson::Subtree>
SubtreeInterop::convertToTilesetJsonSubtree(CesiumGltf::Model&& model) {
  CesiumUtility::ErrorList errors;
  auto* pSubtreeExtension =
      model.getExtension<CesiumGltf::Extension3dTilesSubtree>();
  if (!pSubtreeExtension) {
    errors.emplaceError("Input glTF model did not contain `3DTILES_subtree` "
                        "extension.");
    return Result<CesiumTilesetJson::Subtree>(std::move(errors));
  }

  CesiumTilesetJson::Subtree subtree;
  subtree.tileAvailability =
      convertAvailability(std::move(pSubtreeExtension->tileAvailability));
  subtree.childSubtreeAvailability = convertAvailability(
      std::move(pSubtreeExtension->childSubtreeAvailability));
  if (pSubtreeExtension->contentAvailability) {
    subtree.contentAvailability.emplace_back(convertAvailability(
        std::move(*pSubtreeExtension->contentAvailability)));
  }

  const size_t bufferCount = model.buffers.size();
  subtree.buffers.resize(bufferCount);
  for (size_t i = 0; i < bufferCount; i++) {
    Result<CesiumTilesetJson::Buffer> bufferResult =
        convertBuffer(std::move(model.buffers[i]));
    if (bufferResult.value) {
      subtree.buffers[i] = std::move(*bufferResult.value);
    }
    errors.merge(std::move(bufferResult.errors));
  }

  const size_t bufferViewCount = model.bufferViews.size();
  subtree.bufferViews.resize(bufferViewCount);
  for (size_t i = 0; i < bufferViewCount; i++) {
    Result<CesiumTilesetJson::BufferView> bufferViewResult =
        convertBufferView(std::move(model.bufferViews[i]));
    if (bufferViewResult.value) {
      subtree.bufferViews[i] = std::move(*bufferViewResult.value);
    }
    errors.merge(std::move(bufferViewResult.errors));
  }

  return Result<CesiumTilesetJson::Subtree>(
      std::move(subtree),
      std::move(errors));
}

} // namespace Cesium3DTiles
