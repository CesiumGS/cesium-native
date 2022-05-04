#include <Cesium3DTilesSelection/Exp_SubtreeAvailability.h>
#include <CesiumAsync/IAssetResponse.h>
#include <rapidjson/document.h>
#include <gsl/span>
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

            std::vector<std::byte> buffer(
                data.begin(),
                data.begin() + bufferLength);
            return RequestedSubtreeBuffer{bufferIdx, std::move(buffer)};
          });
}

std::optional<SubtreeAvailabitity> createSubtreeAvailability(
    const rapidjson::Document& subtreeJson,
    const std::vector<std::vector<std::byte>>& buffers) {
  // make sure all the required fields exist
  auto tileAvailabilityIt = subtreeJson.FindMember("tileAvailability");
  if (tileAvailabilityIt == subtreeJson.MemberEnd() ||
      !tileAvailabilityIt->value.IsObject()) {
    return std::nullopt;
  }

  auto contentAvailabilityIt = subtreeJson.FindMember("contentAvailability");
  if (contentAvailabilityIt == subtreeJson.MemberEnd() ||
      !contentAvailabilityIt->value.IsArray()) {
    return std::nullopt;
  }

  auto childSubtreeAvailabilityIt =
      subtreeJson.FindMember("childSubtreeAvailability");
  if (childSubtreeAvailabilityIt == subtreeJson.MemberEnd() ||
      !childSubtreeAvailabilityIt->value.IsObject()) {
    return std::nullopt;
  }
}

CesiumAsync::Future<std::optional<SubtreeAvailabitity>> parseJsonSubtree(
    CesiumAsync::AsyncSystem&& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
    std::shared_ptr<spdlog::logger>&& pLogger,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
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
        continue;
      }

      size_t byteLength = byteLengthIt->value.GetUint();

      auto uriIt = bufferJson.FindMember("uri");
      if (uriIt != bufferJson.MemberEnd()) {
        if (uriIt->value.IsString()) {
          requestBuffers.emplace_back(requestBuffer(
              pAssetAccessor,
              asyncSystem,
              i,
              uriIt->value.GetString(),
              byteLength,
              requestHeaders));
        }
      } else if (
          !internalBuffer.empty() && internalBuffer.size() >= byteLength) {
        resolvedBuffers.emplace_back(std::move(internalBuffer));
      }
    }

    // if we have buffers to request, resolve them now and then, create
    // SubtreeAvailability later
    if (!requestBuffers.empty()) {
      return asyncSystem.all(std::move(requestBuffers))
          .thenInWorkerThread([resolvedBuffers = std::move(resolvedBuffers),
                               subtreeJson = std::move(subtreeJson)](
                                  std::vector<RequestedSubtreeBuffer>&&
                                      completedBuffers) mutable {
            for (auto& requestedBuffer : completedBuffers) {
              resolvedBuffers[requestedBuffer.idx] =
                  std::move(requestedBuffer.data);
            }

            return createSubtreeAvailability(subtreeJson, resolvedBuffers);
          });
    }
  }

  return asyncSystem.createResolvedFuture(
      createSubtreeAvailability(subtreeJson, resolvedBuffers));
}

CesiumAsync::Future<std::optional<SubtreeAvailabitity>> parseJsonSubtreeRequest(
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
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailabitity>>(
        std::nullopt);
  }

  return parseJsonSubtree(
      std::move(asyncSystem),
      std::move(pAssetAccessor),
      std::move(pLogger),
      std::move(requestHeaders),
      std::move(subtreeJson),
      {});
}

CesiumAsync::Future<std::optional<SubtreeAvailabitity>>
parseBinarySubtreeRequest(
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
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailabitity>>(
        std::nullopt);
  }

  const SubtreeHeader* header =
      reinterpret_cast<const SubtreeHeader*>(data.data());
  if (header->jsonByteLength > data.size() - headerLength) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "The Subtree file is invalid because it is too "
        "small to include the jsonByteLength specified in its header.");
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailabitity>>(
        std::nullopt);
  }

  if (header->binaryByteLength >
      data.size() - headerLength - header->jsonByteLength) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "The Subtree file is invalid because it is too "
        "small to include the binaryByteLength specified in its header.");
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailabitity>>(
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
    return asyncSystem.createResolvedFuture<std::optional<SubtreeAvailabitity>>(
        std::nullopt);
  }

  // get the internal buffer if there is any
  std::vector<std::byte> internalBuffer;
  if (header->binaryByteLength > 0) {
    auto begin = data.begin() + headerLength + header->jsonByteLength;
    auto end = begin + header->binaryByteLength;
    internalBuffer.insert(internalBuffer.end(), begin, end);
  }

  return parseJsonSubtree(
      std::move(asyncSystem),
      std::move(pAssetAccessor),
      std::move(pLogger),
      std::move(requestHeaders),
      std::move(subtreeJson),
      std::move(internalBuffer));
}

CesiumAsync::Future<std::optional<SubtreeAvailabitity>> parseSubtreeRequest(
    CesiumAsync::AsyncSystem&& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
    std::shared_ptr<spdlog::logger>&& pLogger,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders) {
  const auto* pResponse = pCompletedRequest->response();
  gsl::span<const std::byte> data = pResponse->data();

  // check if this is binary subtree
  bool isBinarySubtree = false;
  if (data.size() >= 4) {
    for (std::size_t i = 0; i < 4; ++i) {
      if (data[i] != static_cast<std::byte>(SUBTREE_MAGIC[i])) {
        isBinarySubtree = true;
        break;
      }
    }
  }

  if (isBinarySubtree) {
    return parseBinarySubtreeRequest(
        std::move(asyncSystem),
        std::move(pAssetAccessor),
        std::move(pLogger),
        std::move(pCompletedRequest),
        std::move(requestHeaders));
  } else {
    return parseJsonSubtreeRequest(
        std::move(asyncSystem),
        std::move(pAssetAccessor),
        std::move(pLogger),
        std::move(pCompletedRequest),
        std::move(requestHeaders));
  }
}
} // namespace

bool SubtreeAvailabitity::isTileAvailable(
    uint32_t tileLevel,
    uint64_t tileMortonId) const noexcept {
  (void)(tileLevel);
  (void)(tileMortonId);
  return false;
}

bool SubtreeAvailabitity::isTileContentAvailable(
    uint32_t tileLevel,
    uint64_t tileMortonId,
    uint64_t contentId) const noexcept {
  (void)(tileLevel);
  (void)(tileMortonId);
  (void)(contentId);
  return false;
}

bool SubtreeAvailabitity::isSubtreeAvailable(
    uint32_t subtreeLevel,
    uint64_t subtreeMortonId) const noexcept {
  (void)(subtreeLevel);
  (void)(subtreeMortonId);
  return false;
}

CesiumAsync::Future<std::optional<SubtreeAvailabitity>>
SubtreeAvailabitity::loadSubtree(
    const TilesetExternals& externals,
    const std::string& subtreeUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  return externals.pAssetAccessor
      ->get(externals.asyncSystem, subtreeUrl, requestHeaders)
      .thenInWorkerThread([asyncSystem = externals.asyncSystem,
                           pAssetAccessor = externals.pAssetAccessor,
                           pLogger = externals.pLogger,
                           requestHeaders = requestHeaders](
                              std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                  pCompletedRequest) mutable {
        const auto* pResponse = pCompletedRequest->response();
        if (!pResponse) {
          return asyncSystem
              .createResolvedFuture<std::optional<SubtreeAvailabitity>>(
                  std::nullopt);
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          return asyncSystem
              .createResolvedFuture<std::optional<SubtreeAvailabitity>>(
                  std::nullopt);
        }

        return parseSubtreeRequest(
            std::move(asyncSystem),
            std::move(pAssetAccessor),
            std::move(pLogger),
            std::move(pCompletedRequest),
            std::move(requestHeaders));
      });
}
} // namespace Cesium3DTilesSelection
