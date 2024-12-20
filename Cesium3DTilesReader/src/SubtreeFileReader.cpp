#include <Cesium3DTiles/Buffer.h>
#include <Cesium3DTiles/Subtree.h>
#include <Cesium3DTilesReader/SubtreeFileReader.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace Cesium3DTiles;
using namespace CesiumAsync;
using namespace CesiumJsonReader;

namespace Cesium3DTilesReader {

SubtreeFileReader::SubtreeFileReader() : _reader() {}

CesiumJsonReader::JsonReaderOptions& SubtreeFileReader::getOptions() {
  return this->_reader.getOptions();
}

const CesiumJsonReader::JsonReaderOptions&
SubtreeFileReader::getOptions() const {
  return this->_reader.getOptions();
}

//! [async-system-store-in-lambda]
Future<ReadJsonResult<Cesium3DTiles::Subtree>> SubtreeFileReader::load(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers) const noexcept {
  return pAssetAccessor->get(asyncSystem, url, headers)
      .thenInWorkerThread([asyncSystem, pAssetAccessor, this](
                              std::shared_ptr<IAssetRequest>&& pRequest) {
        return this->load(asyncSystem, pAssetAccessor, pRequest);
      });
}
//! [async-system-store-in-lambda]

Future<ReadJsonResult<Subtree>> SubtreeFileReader::load(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<IAssetRequest>& pRequest) const noexcept {
  const IAssetResponse* pResponse = pRequest->response();
  if (pResponse == nullptr) {
    ReadJsonResult<Cesium3DTiles::Subtree> result;
    result.errors.emplace_back("Request failed.");
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  uint16_t statusCode = pResponse->statusCode();
  if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
    CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree> result;
    result.errors.emplace_back(
        fmt::format("Request failed with status code {}", statusCode));
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders(
      pRequest->headers().begin(),
      pRequest->headers().end());
  return this->load(
      asyncSystem,
      pAssetAccessor,
      pRequest->url(),
      requestHeaders,
      pResponse->data());
}

namespace {
constexpr const char SUBTREE_MAGIC[] = "subt";
}

Future<ReadJsonResult<Subtree>> SubtreeFileReader::load(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const std::span<const std::byte>& data) const noexcept {
  if (data.size() < 4) {
    CesiumJsonReader::ReadJsonResult<Subtree> result;
    result.errors.emplace_back(fmt::format(
        "Subtree file has only {} bytes, which is too few to be a valid "
        "subtree.",
        data.size()));
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  bool isBinarySubtree = true;
  for (std::size_t i = 0; i < 4; ++i) {
    if (data[i] != static_cast<std::byte>(SUBTREE_MAGIC[i])) {
      isBinarySubtree = false;
      break;
    }
  }

  if (isBinarySubtree) {
    return this
        ->loadBinary(asyncSystem, pAssetAccessor, url, requestHeaders, data);
  } else {
    return this
        ->loadJson(asyncSystem, pAssetAccessor, url, requestHeaders, data);
  }
}

namespace {

struct SubtreeHeader {
  unsigned char magic[4];
  uint32_t version;
  uint64_t jsonByteLength;
  uint64_t binaryByteLength;
};

} // namespace

Future<ReadJsonResult<Subtree>> SubtreeFileReader::loadBinary(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const std::span<const std::byte>& data) const noexcept {
  if (data.size() < sizeof(SubtreeHeader)) {
    CesiumJsonReader::ReadJsonResult<Subtree> result;
    result.errors.emplace_back(fmt::format(
        "The binary Subtree file is invalid because it is too small to include "
        "a Subtree header.",
        data.size()));
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  const SubtreeHeader* header =
      reinterpret_cast<const SubtreeHeader*>(data.data());
  if (header->jsonByteLength > data.size() - sizeof(SubtreeHeader)) {
    CesiumJsonReader::ReadJsonResult<Subtree> result;
    result.errors.emplace_back(fmt::format(
        "The binary Subtree file is invalid because it is too small to include "
        "the jsonByteLength specified in its header.",
        data.size()));
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  if (header->binaryByteLength >
      data.size() - sizeof(SubtreeHeader) - header->jsonByteLength) {
    CesiumJsonReader::ReadJsonResult<Subtree> result;
    result.errors.emplace_back(fmt::format(
        "The binary Subtree file is invalid because it is too small to include "
        "the binaryByteLength specified in its header.",
        data.size()));
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  ReadJsonResult<Subtree> result = this->_reader.readFromJson(
      data.subspan(sizeof(SubtreeHeader), header->jsonByteLength));

  if (result.value) {
    std::span<const std::byte> binaryChunk = data.subspan(
        sizeof(SubtreeHeader) + header->jsonByteLength,
        header->binaryByteLength);

    if (binaryChunk.size() > 0) {
      if (result.value->buffers.empty()) {
        result.errors.emplace_back(
            "Subtree has a binary chunk but the JSON does "
            "not define any buffers.");
        return asyncSystem.createResolvedFuture(std::move(result));
      }

      Buffer& buffer = result.value->buffers[0];
      if (buffer.uri) {
        result.errors.emplace_back(
            "Subtree has a binary chunk but the first buffer "
            "in the JSON chunk also has a 'uri'.");
        return asyncSystem.createResolvedFuture(std::move(result));
      }

      const int64_t binaryChunkSize = static_cast<int64_t>(binaryChunk.size());

      // We allow - but don't require - 8-byte padding.
      int64_t maxPaddingBytes = 0;
      int64_t paddingRemainder = buffer.byteLength % 8;
      if (paddingRemainder > 0) {
        maxPaddingBytes = 8 - paddingRemainder;
      }

      if (buffer.byteLength > binaryChunkSize ||
          buffer.byteLength + maxPaddingBytes < binaryChunkSize) {
        result.errors.emplace_back(
            "Subtree binary chunk size does not match the "
            "size of the first buffer in the JSON chunk.");
        return asyncSystem.createResolvedFuture(std::move(result));
      }

      buffer.cesium.data = std::vector<std::byte>(
          binaryChunk.begin(),
          binaryChunk.begin() + buffer.byteLength);
    }
  }

  return postprocess(
      asyncSystem,
      pAssetAccessor,
      url,
      requestHeaders,
      std::move(result));
}

CesiumAsync::Future<ReadJsonResult<Subtree>> SubtreeFileReader::loadJson(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const std::span<const std::byte>& data) const noexcept {
  ReadJsonResult<Subtree> result = this->_reader.readFromJson(data);
  return postprocess(
      asyncSystem,
      pAssetAccessor,
      url,
      requestHeaders,
      std::move(result));
}

namespace {

struct RequestedSubtreeBuffer {
  size_t index;
  std::vector<std::byte> data;
};

CesiumAsync::Future<RequestedSubtreeBuffer> requestBuffer(
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const CesiumAsync::AsyncSystem& asyncSystem,
    size_t bufferIdx,
    std::string&& subtreeUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  return pAssetAccessor->get(asyncSystem, subtreeUrl, requestHeaders)
      .thenInWorkerThread(
          [bufferIdx](
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

            const std::span<const std::byte>& data = pResponse->data();
            return RequestedSubtreeBuffer{
                bufferIdx,
                std::vector<std::byte>(data.begin(), data.end())};
          });
}

} // namespace

Future<ReadJsonResult<Subtree>> SubtreeFileReader::postprocess(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    ReadJsonResult<Subtree>&& loaded) const noexcept {
  if (!loaded.value) {
    return asyncSystem.createResolvedFuture(std::move(loaded));
  }

  // Load any external buffers
  std::vector<Future<RequestedSubtreeBuffer>> bufferRequests;
  const std::vector<Buffer>& buffers = loaded.value->buffers;
  for (size_t i = 0; i < buffers.size(); ++i) {
    const Buffer& buffer = buffers[i];
    if (buffer.uri && !buffer.uri->empty()) {
      std::string bufferUrl = CesiumUtility::Uri::resolve(url, *buffer.uri);
      bufferRequests.emplace_back(requestBuffer(
          pAssetAccessor,
          asyncSystem,
          i,
          std::move(bufferUrl),
          requestHeaders));
    }
  }

  if (!bufferRequests.empty()) {
    return asyncSystem.all(std::move(bufferRequests))
        .thenInMainThread(
            [loaded = std::move(loaded)](std::vector<RequestedSubtreeBuffer>&&
                                             completedBuffers) mutable {
              for (RequestedSubtreeBuffer& completedBuffer : completedBuffers) {
                Buffer& buffer = loaded.value->buffers[completedBuffer.index];
                if (buffer.byteLength >
                    static_cast<int64_t>(completedBuffer.data.size())) {
                  loaded.warnings.emplace_back(fmt::format(
                      "Buffer byteLength ({}) is greater than the size of the "
                      "downloaded resource ({} bytes). The byteLength will be "
                      "updated to match.",
                      buffer.byteLength,
                      completedBuffer.data.size()));
                  buffer.byteLength =
                      static_cast<int64_t>(completedBuffer.data.size());
                }
                buffer.cesium.data = std::move(completedBuffer.data);
              }

              return std::move(loaded);
            });
  }

  return asyncSystem.createResolvedFuture(std::move(loaded));
}

} // namespace Cesium3DTilesReader
