#include <Cesium3DTilesReader/SubtreeFileReader.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Uri.h>

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

Future<ReadJsonResult<Subtree>> SubtreeFileReader::load(
    const AsyncSystem& asyncSystem,
    const std::string& baseUrl,
    const CesiumAsync::IAssetResponse* baseResponse,
    const UrlResponseDataMap& additionalResponses) const noexcept {
  assert(baseResponse);
  return this
      ->load(asyncSystem, baseUrl, additionalResponses, baseResponse->data());
}

namespace {
constexpr const char SUBTREE_MAGIC[] = "subt";
}

Future<ReadJsonResult<Subtree>> SubtreeFileReader::load(
    const AsyncSystem& asyncSystem,
    const std::string& baseUrl,
    const UrlResponseDataMap& additionalResponses,
    const gsl::span<const std::byte>& data) const noexcept {

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
    return this->loadBinary(asyncSystem, baseUrl, data, additionalResponses);
  } else {
    return this->loadJson(asyncSystem, baseUrl, data, additionalResponses);
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
    const std::string& baseUrl,
    const gsl::span<const std::byte>& data,
    const CesiumAsync::UrlResponseDataMap& additionalResponses) const noexcept {
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
    gsl::span<const std::byte> binaryChunk = data.subspan(
        sizeof(SubtreeHeader) + header->jsonByteLength,
        header->binaryByteLength);

    if (result.value->buffers.empty()) {
      result.errors.emplace_back("Subtree has a binary chunk but the JSON does "
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
      result.errors.emplace_back("Subtree binary chunk size does not match the "
                                 "size of the first buffer in the JSON chunk.");
      return asyncSystem.createResolvedFuture(std::move(result));
    }

    buffer.cesium.data = std::vector<std::byte>(
        binaryChunk.begin(),
        binaryChunk.begin() + buffer.byteLength);
  }

  return postprocess(
      asyncSystem,
      baseUrl,
      additionalResponses,
      std::move(result));
}

CesiumAsync::Future<ReadJsonResult<Subtree>> SubtreeFileReader::loadJson(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& baseUrl,
    const gsl::span<const std::byte>& data,
    const CesiumAsync::UrlResponseDataMap& additionalResponses) const noexcept {
  ReadJsonResult<Subtree> result = this->_reader.readFromJson(data);
  return postprocess(
      asyncSystem,
      baseUrl,
      additionalResponses,
      std::move(result));
}

namespace {

struct RequestedSubtreeBuffer {
  size_t index;
  std::vector<std::byte> data;
};

RequestedSubtreeBuffer requestBuffer(
    size_t bufferIdx,
    uint16_t statusCode,
    const gsl::span<const std::byte>& data) {
  if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
    return RequestedSubtreeBuffer{bufferIdx, {}};
  }

  return RequestedSubtreeBuffer{
      bufferIdx,
      std::vector<std::byte>(data.begin(), data.end())};
}

} // namespace

Future<ReadJsonResult<Subtree>> SubtreeFileReader::postprocess(
    const AsyncSystem& asyncSystem,
    const std::string& baseUrl,
    const CesiumAsync::UrlResponseDataMap& additionalResponses,
    ReadJsonResult<Subtree>&& loaded) const noexcept {
  if (!loaded.value) {
    return asyncSystem.createResolvedFuture(std::move(loaded));
  }

  // Load any external buffers
  std::vector<RequestedSubtreeBuffer> bufferRequests;
  const std::vector<Buffer>& buffers = loaded.value->buffers;
  for (size_t i = 0; i < buffers.size(); ++i) {
    const Buffer& buffer = buffers[i];
    if (buffer.uri && !buffer.uri->empty()) {
      std::string bufferUrl = CesiumUtility::Uri::resolve(baseUrl, *buffer.uri);

      // Find this buffer in our responses
      auto bufferUrlIt = additionalResponses.find(bufferUrl);
      if (bufferUrlIt == additionalResponses.end()) {
        // We need to request this buffer
        loaded.urlNeeded = bufferUrl;
        return asyncSystem.createResolvedFuture(std::move(loaded));
      }

      assert(bufferUrlIt->second.pResponse);

      bufferRequests.emplace_back(requestBuffer(
          i,
          bufferUrlIt->second.pResponse->statusCode(),
          bufferUrlIt->second.pResponse->data()));
    }
  }

  if (!bufferRequests.empty()) {
    return asyncSystem.runInMainThread(
        [loaded = std::move(loaded),
         completedBuffers = std::move(bufferRequests)]() mutable {
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
