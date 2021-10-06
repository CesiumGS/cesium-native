
#include "AvailabilitySubtreeContent.h"

#include "Cesium3DTilesSelection/TileContentFactory.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/HttpHeaders.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeometry/AvailabilitySubtree.h"
#include "CesiumUtility/Uri.h"

#include <gsl/span>
#include <rapidjson/document.h>

#include <cstring>
#include <optional>
#include <string>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeometry;

namespace {
struct SubtreeHeader {
  unsigned char magic[4];
  uint32_t version;
  uint64_t jsonByteLength;
  uint64_t binaryByteLength;
};

static constexpr size_t headerLength = sizeof(SubtreeHeader);

struct SubtreeBuffer {
  std::string name;
  std::optional<std::string> uri;
  uint32_t byteLength;
};

} // namespace

namespace Cesium3DTilesSelection {

static Future<std::vector<std::byte>> resolveSubtreeBuffer(
    SubtreeBuffer&& buffer,
    const std::string& url,
    const gsl::span<const std::byte>& binaryData,
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::vector<IAssetAccessor::THeader>& headers,
    const std::shared_ptr<spdlog::logger>& /*pLogger*/) {

  if (buffer.uri) {
    // Using external buffer.
    std::string fullBufferUri =
        CesiumUtility::Uri::resolve(url, *buffer.uri, false);
    return pAssetAccessor->requestAsset(asyncSystem, fullBufferUri, headers)
        .thenInWorkerThread([buffer = std::move(buffer)](
                                std::shared_ptr<IAssetRequest>&& pRequest) {
          if (pRequest->response()) {
            const gsl::span<const std::byte>& data =
                pRequest->response()->data();

            if (data.size() == buffer.byteLength) {
              return std::vector<std::byte>(data.begin(), data.end());
            }
          }

          return std::vector<std::byte>();
        });
  } else {
    // Using internal buffer.
    return asyncSystem.createResolvedFuture(
        std::vector<std::byte>(binaryData.begin(), binaryData.end()));
  }
}

Future<std::unique_ptr<TileContentLoadResult>>
load(const TileContentLoadInput& input) {
  const AsyncSystem& asyncSystem = input.asyncSystem;
  const std::shared_ptr<spdlog::logger>& pLogger = input.pLogger;
  const std::string& url = input.pRequest->url();
  const gsl::span<const std::byte>& data = input.pRequest->response()->data();
  const std::shared_ptr<IAssetAccessor>& pAssetAccessor = input.pAssetAccessor;
  const HttpHeaders& headers = input.pRequest->headers();
  // TODO: Can we avoid this copy conversion?
  std::vector<IAssetAccessor::THeader> tHeaders(headers.begin(), headers.end());

  if (data.size() < headerLength) {
    throw std::runtime_error("The Subtree file is invalid because it is too "
                             "small to include a Subtree header.");
  }

  const SubtreeHeader* header =
      reinterpret_cast<const SubtreeHeader*>(data.data());

  rapidjson::Document document;
  document.Parse(
      reinterpret_cast<const char*>(data.data() + headerLength),
      header->jsonByteLength);
  if (document.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        document.GetParseError(),
        document.GetErrorOffset());
    return asyncSystem
        .createResolvedFuture<std::unique_ptr<TileContentLoadResult>>(nullptr);
  }

  AvailabilitySubtree subtreeResult;

  // Parse buffer views.
  std::vector<SubtreeBufferView> subtreeBufferViews;
  auto bufferViewsIt = document.FindMember("bufferViews");
  if (bufferViewsIt != document.MemberEnd() && bufferViewsIt->value.IsArray()) {
    auto bufferViewsArray = bufferViewsIt->value.GetArray();
    uint32_t bufferViewsCount = bufferViewsArray.Size();
    subtreeBufferViews.reserve(bufferViewsCount);

    for (uint32_t i = 0; i < bufferViewsCount; ++i) {
      SubtreeBufferView subtreeBufferView;

      const auto& bufferViewIt = bufferViewsArray[i];

      if (bufferViewIt.IsObject()) {
        auto bufferView = bufferViewIt.GetObject();

        auto bufferIt = bufferViewIt.FindMember("buffer");
        auto byteOffsetIt = bufferViewIt.FindMember("byteOffset");
        auto byteLengthIt = bufferViewIt.FindMember("byteLength");

        if (bufferIt != bufferView.MemberEnd() && bufferIt->value.IsUint() &&
            byteOffsetIt != bufferView.MemberEnd() &&
            byteOffsetIt->value.IsUint() &&
            byteLengthIt != bufferView.MemberEnd() &&
            byteLengthIt->value.IsUint()) {
          subtreeBufferView.buffer =
              static_cast<uint8_t>(bufferIt->value.GetUint());
          subtreeBufferView.byteOffset = byteOffsetIt->value.GetUint();
          subtreeBufferView.byteLength = byteLengthIt->value.GetUint();

          subtreeBufferViews.push_back(std::move(subtreeBufferView));
        }
      }
    }
  }

  // Parse availability views.
  auto resolveAvailabilityView =
      [&subtreeBufferViews](const rapidjson::Value::Object& availabilityObject)
      -> AvailabilityView {
    auto constantIt = availabilityObject.FindMember("constant");
    auto bufferViewIt = availabilityObject.FindMember("bufferView");

    if (constantIt != availabilityObject.MemberEnd() &&
        constantIt->value.IsUint()) {

      return ConstantAvailability{
          static_cast<uint8_t>(constantIt->value.GetUint())};

    } else if (
        bufferViewIt != availabilityObject.MemberEnd() &&
        bufferViewIt->value.IsUint()) {

      uint32_t bufferViewId = bufferViewIt->value.GetUint();
      // TODO: assumes buffer views are only used once each, is that fair?
      if (bufferViewId < subtreeBufferViews.size()) {
        return std::move(subtreeBufferViews[bufferViewId]);
      }
    }

    return ConstantAvailability{0};
  };

  auto tileAvailabilityIt = document.FindMember("tileAvailability");
  auto contentAvailability = document.FindMember("contentAvailability");
  auto subtreeAvailability = document.FindMember("childSubtreeAvailability");

  if (tileAvailabilityIt != document.MemberEnd() &&
      tileAvailabilityIt->value.IsObject() &&
      contentAvailability != document.MemberEnd() &&
      contentAvailability->value.IsObject() &&
      subtreeAvailability != document.MemberEnd() &&
      subtreeAvailability->value.IsObject()) {

    subtreeResult.tileAvailability = std::move(
        resolveAvailabilityView(tileAvailabilityIt->value.GetObject()));
    subtreeResult.contentAvailability = std::move(
        resolveAvailabilityView(contentAvailability->value.GetObject()));
    subtreeResult.subtreeAvailability = std::move(
        resolveAvailabilityView(subtreeAvailability->value.GetObject()));
  }

  // Parse buffers, request external buffers as needed.
  gsl::span<const std::byte> binaryData = data.subspan(
      headerLength + header->jsonByteLength,
      header->binaryByteLength);

  std::vector<Future<std::vector<std::byte>>> futureBuffers;

  auto buffersIt = document.FindMember("buffers");
  if (buffersIt != document.MemberEnd() && buffersIt->value.IsArray()) {

    auto buffersArray = buffersIt->value.GetArray();
    uint32_t bufferCount = buffersArray.Size();
    futureBuffers.reserve(bufferCount);

    for (uint32_t i = 0; i < bufferCount; ++i) {
      const auto& bufferIt = buffersArray[i];
      if (bufferIt.IsObject()) {
        SubtreeBuffer subtreeBuffer;

        auto buffer = bufferIt.GetObject();
        auto nameIt = buffer.FindMember("name");
        if (nameIt != buffer.MemberEnd() && nameIt->value.IsString()) {
          subtreeBuffer.name = nameIt->value.GetString();
        }

        auto uriIt = buffer.FindMember("uri");
        if (uriIt != buffer.MemberEnd() && uriIt->value.IsString()) {
          subtreeBuffer.uri = uriIt->value.GetString();
        }

        auto byteLengthIt = buffer.FindMember("byteLength");
        if (byteLengthIt != buffer.MemberEnd() &&
            byteLengthIt->value.IsUint()) {
          subtreeBuffer.byteLength = byteLengthIt->value.GetUint();
        }

        futureBuffers.push_back(resolveSubtreeBuffer(
            std::move(subtreeBuffer),
            url,
            binaryData,
            asyncSystem,
            pAssetAccessor,
            tHeaders,
            pLogger));
      }
    }
  }

  // TODO: is this a bit hacky, can we change some terminology to make this
  // feel more legitimate? Currently this works similar to CompositeContent.

  // The input needed to actually now load the tile content.
  TileContentLoadInput contentInput(
      input.asyncSystem,
      input.pLogger,
      input.pAssetAccessor,
      input.pRequest,
      nullptr,
      input.tileID,
      input.tileBoundingVolume,
      input.tileContentBoundingVolume,
      input.tileRefine,
      input.tileGeometricError,
      input.tileTransform,
      input.contentOptions);

  return asyncSystem.all(std::move(futureBuffers))
      .thenInWorkerThread(
          [asyncSystem,
           contentInput = std::move(contentInput),
           subtreeResult = std::move(subtreeResult)](
              std::vector<std::vector<std::byte>>&& buffers) mutable {
            subtreeResult.buffers = std::move(buffers);

            return TileContentFactory::createContent(contentInput)
                .thenInWorkerThread(
                    [subtreeResult = std::move(subtreeResult)](
                        std::unique_ptr<TileContentLoadResult>&& pResult) {
                      pResult->subtreeLoadResult = std::move(subtreeResult);
                      return std::move(pResult);
                    });
          });
}

} // namespace Cesium3DTilesSelection