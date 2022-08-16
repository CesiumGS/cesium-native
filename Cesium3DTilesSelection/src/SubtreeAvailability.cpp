#include "SubtreeAvailability.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Uri.h>

#include <gsl/span>
#include <rapidjson/document.h>

#include <optional>
#include <string>

namespace Cesium3DTilesSelection {
namespace {
constexpr const char* const SUBTREE_MAGIC = "subt";

struct SubtreeHeader {
  unsigned char magic[4];
  uint32_t version;
  uint64_t jsonByteLength;
  uint64_t binaryByteLength;
};

struct RequestedSubtreeBuffer {
  size_t idx;
  std::vector<std::byte> data;
};

struct SubtreeBufferView {
  size_t bufferIdx;
  size_t byteOffset;
  size_t byteLength;
};

CesiumAsync::Future<RequestedSubtreeBuffer> requestBuffer(
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const CesiumAsync::AsyncSystem& asyncSystem,
    size_t bufferIdx,
    std::string&& subtreeUrl,
    size_t bufferLength,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  return pAssetAccessor->get(asyncSystem, subtreeUrl, requestHeaders)
      .thenInWorkerThread(
          [bufferIdx, bufferLength](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
            const CesiumAsync::IAssetResponse* pResponse =
                pCompletedRequest->response();
            if (!pResponse) {
              return RequestedSubtreeBuffer{bufferIdx, {}};
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              return RequestedSubtreeBuffer{bufferIdx, {}};
            }

            auto data = pResponse->data();
            if (data.size() < bufferLength) {
              return RequestedSubtreeBuffer{bufferIdx, {}};
            }

            using vector_diff_type =
                typename std::vector<std::byte>::difference_type;
            std::vector<std::byte> buffer(
                data.begin(),
                data.begin() + static_cast<vector_diff_type>(bufferLength));
            return RequestedSubtreeBuffer{bufferIdx, std::move(buffer)};
          });
}

std::optional<AvailabilityView> parseAvailabilityView(
    const rapidjson::Value& availabilityJson,
    const std::vector<std::vector<std::byte>>& buffers,
    const std::vector<SubtreeBufferView>& bufferViews) {
  auto constantIt = availabilityJson.FindMember("constant");
  if (constantIt != availabilityJson.MemberEnd() &&
      constantIt->value.IsUint()) {
    return SubtreeConstantAvailability{constantIt->value.GetUint() == 1U};
  }

  auto bitStreamIt = availabilityJson.FindMember("bitstream");
  if (bitStreamIt == availabilityJson.MemberEnd()) {
    // old version uses bufferView property instead of bitstream. Same semantic
    // either way
    bitStreamIt = availabilityJson.FindMember("bufferView");
  }
  if (bitStreamIt != availabilityJson.MemberEnd() &&
      bitStreamIt->value.IsUint()) {
    uint32_t bufferViewIdx = bitStreamIt->value.GetUint();
    if (bufferViewIdx < bufferViews.size()) {
      const SubtreeBufferView& bufferView = bufferViews[bufferViewIdx];

      if (bufferView.bufferIdx < buffers.size()) {
        const std::vector<std::byte>& buffer = buffers[bufferView.bufferIdx];
        if (bufferView.byteOffset + bufferView.byteLength <= buffer.size()) {
          return SubtreeBufferViewAvailability{gsl::span<const std::byte>(
              buffer.data() + bufferView.byteOffset,
              bufferView.byteLength)};
        }
      }
    }
  }

  return std::nullopt;
}

std::optional<SubtreeAvailability> createSubtreeAvailability(
    uint32_t powerOf2,
    const rapidjson::Document& subtreeJson,
    std::vector<std::vector<std::byte>>&& buffers) {
  // make sure all the required fields exist
  auto tileAvailabilityIt = subtreeJson.FindMember("tileAvailability");
  if (tileAvailabilityIt == subtreeJson.MemberEnd() ||
      !tileAvailabilityIt->value.IsObject()) {
    return std::nullopt;
  }

  auto contentAvailabilityIt = subtreeJson.FindMember("contentAvailability");
  if (contentAvailabilityIt == subtreeJson.MemberEnd() ||
      (!contentAvailabilityIt->value.IsArray() &&
       !contentAvailabilityIt->value.IsObject())) {
    return std::nullopt;
  }

  auto childSubtreeAvailabilityIt =
      subtreeJson.FindMember("childSubtreeAvailability");
  if (childSubtreeAvailabilityIt == subtreeJson.MemberEnd() ||
      !childSubtreeAvailabilityIt->value.IsObject()) {
    return std::nullopt;
  }

  std::vector<SubtreeBufferView> bufferViews;
  auto bufferViewIt = subtreeJson.FindMember("bufferViews");
  if (bufferViewIt != subtreeJson.MemberEnd() &&
      bufferViewIt->value.IsArray()) {
    bufferViews.resize(bufferViewIt->value.Size());
    for (rapidjson::SizeType i = 0; i < bufferViewIt->value.Size(); ++i) {
      const auto& bufferViewJson = bufferViewIt->value[i];
      auto bufferIdxIt = bufferViewJson.FindMember("buffer");
      auto byteOffsetIt = bufferViewJson.FindMember("byteOffset");
      auto byteLengthIt = bufferViewJson.FindMember("byteLength");

      if (bufferIdxIt == bufferViewJson.MemberEnd() ||
          !bufferIdxIt->value.IsUint()) {
        return std::nullopt;
      }

      if (byteOffsetIt == bufferViewJson.MemberEnd() ||
          !byteOffsetIt->value.IsUint()) {
        return std::nullopt;
      }

      if (byteLengthIt == bufferViewJson.MemberEnd() ||
          !byteLengthIt->value.IsUint()) {
        return std::nullopt;
      }

      bufferViews[i].bufferIdx = bufferIdxIt->value.GetUint();
      bufferViews[i].byteOffset = byteOffsetIt->value.GetUint();
      bufferViews[i].byteLength = byteLengthIt->value.GetUint();
    }
  }

  auto tileAvailability =
      parseAvailabilityView(tileAvailabilityIt->value, buffers, bufferViews);
  if (!tileAvailability) {
    return std::nullopt;
  }

  auto childSubtreeAvailability = parseAvailabilityView(
      childSubtreeAvailabilityIt->value,
      buffers,
      bufferViews);
  if (!childSubtreeAvailability) {
    return std::nullopt;
  }

  std::vector<AvailabilityView> contentAvailability;
  if (contentAvailabilityIt->value.IsArray()) {
    contentAvailability.reserve(contentAvailabilityIt->value.Size());
    for (const auto& contentAvailabilityJson :
         contentAvailabilityIt->value.GetArray()) {
      auto availability =
          parseAvailabilityView(contentAvailabilityJson, buffers, bufferViews);
      if (!availability) {
        return std::nullopt;
      }

      contentAvailability.emplace_back(*availability);
    }
  } else {
    auto availability = parseAvailabilityView(
        contentAvailabilityIt->value,
        buffers,
        bufferViews);
    if (!availability) {
      return std::nullopt;
    }

    contentAvailability.emplace_back(*availability);
  }

  return SubtreeAvailability{
      powerOf2,
      *tileAvailability,
      *childSubtreeAvailability,
      std::move(contentAvailability),
      std::move(buffers)};
}

CesiumAsync::Future<std::optional<SubtreeAvailability>> parseJsonSubtree(
    uint32_t powerOf2,
    CesiumAsync::AsyncSystem&& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
    std::shared_ptr<spdlog::logger>&& pLogger,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
    const std::string& baseUrl,
    rapidjson::Document&& subtreeJson,
    std::vector<std::byte>&& internalBuffer) {
  // resolve all the buffers
  std::vector<std::vector<std::byte>> resolvedBuffers;
  auto bufferIt = subtreeJson.FindMember("buffers");
  if (bufferIt != subtreeJson.MemberEnd() && bufferIt->value.IsArray()) {
    const auto& arrayBufferJsons = bufferIt->value.GetArray();
    resolvedBuffers.resize(arrayBufferJsons.Size());

    std::vector<CesiumAsync::Future<RequestedSubtreeBuffer>> requestBuffers;
    for (rapidjson::SizeType i = 0; i < arrayBufferJsons.Size(); ++i) {
      const auto& bufferJson = arrayBufferJsons[i];
      auto byteLengthIt = bufferJson.FindMember("byteLength");
      if (byteLengthIt == bufferJson.MemberEnd() ||
          !byteLengthIt->value.IsUint()) {
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "Subtree Buffer requires byteLength property.");
        return asyncSystem
            .createResolvedFuture<std::optional<SubtreeAvailability>>(
                std::nullopt);
      }

      size_t byteLength = byteLengthIt->value.GetUint();

      auto uriIt = bufferJson.FindMember("uri");
      if (uriIt != bufferJson.MemberEnd()) {
        if (!uriIt->value.IsString()) {
          SPDLOG_LOGGER_ERROR(
              pLogger,
              "Subtree Buffer has uri field but it's not string.");
          return asyncSystem
              .createResolvedFuture<std::optional<SubtreeAvailability>>(
                  std::nullopt);
        }

        std::string bufferUrl =
            CesiumUtility::Uri::resolve(baseUrl, uriIt->value.GetString());
        requestBuffers.emplace_back(requestBuffer(
            pAssetAccessor,
            asyncSystem,
            i,
            std::move(bufferUrl),
            byteLength,
            requestHeaders));
      } else if (
          !internalBuffer.empty() && internalBuffer.size() >= byteLength) {
        resolvedBuffers[i] = std::move(internalBuffer);
      }
    }

    // if we have buffers to request, resolve them now and then, create
    // SubtreeAvailability later
    if (!requestBuffers.empty()) {
      return asyncSystem.all(std::move(requestBuffers))
          .thenInWorkerThread([powerOf2,
                               resolvedBuffers = std::move(resolvedBuffers),
                               subtreeJson = std::move(subtreeJson)](
                                  std::vector<RequestedSubtreeBuffer>&&
                                      completedBuffers) mutable {
            for (auto& requestedBuffer : completedBuffers) {
              resolvedBuffers[requestedBuffer.idx] =
                  std::move(requestedBuffer.data);
            }

            return createSubtreeAvailability(
                powerOf2,
                subtreeJson,
                std::move(resolvedBuffers));
          });
    }
  }

  return asyncSystem.createResolvedFuture(createSubtreeAvailability(
      powerOf2,
      subtreeJson,
      std::move(resolvedBuffers)));
}

CesiumAsync::Future<std::optional<SubtreeAvailability>> parseJsonSubtreeRequest(
    uint32_t powerOf2,
    CesiumAsync::AsyncSystem&& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
    std::shared_ptr<spdlog::logger>&& pLogger,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders) {
  const auto* pResponse = pCompletedRequest->response();
  gsl::span<const std::byte> data = pResponse->data();

  rapidjson::Document subtreeJson;
  subtreeJson.Parse(reinterpret_cast<const char*>(data.data()), data.size());
  if (subtreeJson.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        subtreeJson.GetParseError(),
        subtreeJson.GetErrorOffset());
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailability>>(
        std::nullopt);
  }

  return parseJsonSubtree(
      powerOf2,
      std::move(asyncSystem),
      std::move(pAssetAccessor),
      std::move(pLogger),
      std::move(requestHeaders),
      pCompletedRequest->url(),
      std::move(subtreeJson),
      {});
}

CesiumAsync::Future<std::optional<SubtreeAvailability>>
parseBinarySubtreeRequest(
    uint32_t powerOf2,
    CesiumAsync::AsyncSystem&& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
    std::shared_ptr<spdlog::logger>&& pLogger,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders) {
  const auto* pResponse = pCompletedRequest->response();
  gsl::span<const std::byte> data = pResponse->data();

  size_t headerLength = sizeof(SubtreeHeader);
  if (data.size() < headerLength) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "The Subtree file is invalid because it is too "
        "small to include a Subtree header.");
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailability>>(
        std::nullopt);
  }

  const SubtreeHeader* header =
      reinterpret_cast<const SubtreeHeader*>(data.data());
  if (header->jsonByteLength > data.size() - headerLength) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "The Subtree file is invalid because it is too "
        "small to include the jsonByteLength specified in its header.");
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailability>>(
        std::nullopt);
  }

  if (header->binaryByteLength >
      data.size() - headerLength - header->jsonByteLength) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "The Subtree file is invalid because it is too "
        "small to include the binaryByteLength specified in its header.");
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailability>>(
        std::nullopt);
  }

  rapidjson::Document subtreeJson;
  subtreeJson.Parse(
      reinterpret_cast<const char*>(data.data() + headerLength),
      header->jsonByteLength);
  if (subtreeJson.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        subtreeJson.GetParseError(),
        subtreeJson.GetErrorOffset());
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailability>>(
        std::nullopt);
  }

  // get the internal buffer if there is any
  std::vector<std::byte> internalBuffer;
  if (header->binaryByteLength > 0) {
    using vector_diff_type = typename std::vector<std::byte>::difference_type;
    auto begin = data.begin() + static_cast<vector_diff_type>(headerLength) +
                 static_cast<vector_diff_type>(header->jsonByteLength);
    auto end = begin + static_cast<vector_diff_type>(header->binaryByteLength);
    internalBuffer.insert(internalBuffer.end(), begin, end);
  }

  return parseJsonSubtree(
      powerOf2,
      std::move(asyncSystem),
      std::move(pAssetAccessor),
      std::move(pLogger),
      std::move(requestHeaders),
      pCompletedRequest->url(),
      std::move(subtreeJson),
      std::move(internalBuffer));
}

CesiumAsync::Future<std::optional<SubtreeAvailability>> parseSubtreeRequest(
    uint32_t powerOf2,
    CesiumAsync::AsyncSystem&& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
    std::shared_ptr<spdlog::logger>&& pLogger,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders) {
  const auto* pResponse = pCompletedRequest->response();
  gsl::span<const std::byte> data = pResponse->data();

  // check if this is binary subtree
  bool isBinarySubtree = true;
  if (data.size() >= 4) {
    for (std::size_t i = 0; i < 4; ++i) {
      if (data[i] != static_cast<std::byte>(SUBTREE_MAGIC[i])) {
        isBinarySubtree = false;
        break;
      }
    }
  }

  if (isBinarySubtree) {
    return parseBinarySubtreeRequest(
        powerOf2,
        std::move(asyncSystem),
        std::move(pAssetAccessor),
        std::move(pLogger),
        std::move(pCompletedRequest),
        std::move(requestHeaders));
  } else {
    return parseJsonSubtreeRequest(
        powerOf2,
        std::move(asyncSystem),
        std::move(pAssetAccessor),
        std::move(pLogger),
        std::move(pCompletedRequest),
        std::move(requestHeaders));
  }
}
} // namespace

SubtreeAvailability::SubtreeAvailability(
    uint32_t powerOf2,
    AvailabilityView tileAvailability,
    AvailabilityView subtreeAvailability,
    std::vector<AvailabilityView>&& contentAvailability,
    std::vector<std::vector<std::byte>>&& buffers)
    : _childCount{1U << powerOf2},
      _powerOf2{powerOf2},
      _tileAvailability{tileAvailability},
      _subtreeAvailability{subtreeAvailability},
      _contentAvailability{std::move(contentAvailability)},
      _buffers{std::move(buffers)} {
  assert(
      (this->_childCount == 4 || this->_childCount == 8) &&
      "Only support quadtree and octree");
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

CesiumAsync::Future<std::optional<SubtreeAvailability>>
SubtreeAvailability::loadSubtree(
    uint32_t powerOf2,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& subtreeUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  return pAssetAccessor->get(asyncSystem, subtreeUrl, requestHeaders)
      .thenInWorkerThread([powerOf2,
                           asyncSystem = asyncSystem,
                           pAssetAccessor = pAssetAccessor,
                           pLogger = pLogger,
                           requestHeaders = requestHeaders](
                              std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                  pCompletedRequest) mutable {
        const auto* pResponse = pCompletedRequest->response();
        if (!pResponse) {
          return asyncSystem
              .createResolvedFuture<std::optional<SubtreeAvailability>>(
                  std::nullopt);
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          return asyncSystem
              .createResolvedFuture<std::optional<SubtreeAvailability>>(
                  std::nullopt);
        }

        return parseSubtreeRequest(
            powerOf2,
            std::move(asyncSystem),
            std::move(pAssetAccessor),
            std::move(pLogger),
            std::move(pCompletedRequest),
            std::move(requestHeaders));
      });
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
} // namespace Cesium3DTilesSelection
