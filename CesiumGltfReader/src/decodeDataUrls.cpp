#include "decodeDataUrls.h"

#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/Model.h>
#include <CesiumUtility/Tracing.h>

#include <modp_b64.h>

#include <cstddef>

namespace CesiumGltfReader {

namespace {

std::vector<std::byte> decodeBase64(gsl::span<const std::byte> data) {
  CESIUM_TRACE("CesiumGltfReader::decodeBase64");
  std::vector<std::byte> result(modp_b64_decode_len(data.size()));

  const size_t resultLength = modp_b64_decode(
      reinterpret_cast<char*>(result.data()),
      reinterpret_cast<const char*>(data.data()),
      data.size());
  if (resultLength == size_t(-1)) {
    result.clear();
    result.shrink_to_fit();
  } else {
    result.resize(resultLength);
  }

  return result;
}

struct DecodeResult {
  std::string mimeType;
  std::vector<std::byte> data;
};

std::optional<DecodeResult> tryDecode(const std::string& uri) {
  constexpr std::string_view dataPrefix = "data:";
  constexpr size_t dataPrefixLength = dataPrefix.size();

  constexpr std::string_view base64Indicator = ";base64";
  constexpr size_t base64IndicatorLength = base64Indicator.size();

  if (uri.substr(0, dataPrefixLength) != dataPrefix) {
    return std::nullopt;
  }

  const size_t dataDelimeter = uri.find(',', dataPrefixLength);
  if (dataDelimeter == std::string::npos) {
    return std::nullopt;
  }

  bool isBase64Encoded = false;

  DecodeResult result;

  result.mimeType =
      uri.substr(dataPrefixLength, dataDelimeter - dataPrefixLength);
  if (result.mimeType.size() >= base64IndicatorLength &&
      result.mimeType.substr(
          result.mimeType.size() - base64IndicatorLength,
          base64IndicatorLength) == base64Indicator) {
    isBase64Encoded = true;
    result.mimeType = result.mimeType.substr(
        0,
        result.mimeType.size() - base64IndicatorLength);
  }

  const gsl::span<const std::byte> data(
      reinterpret_cast<const std::byte*>(uri.data()) + dataDelimeter + 1,
      uri.size() - dataDelimeter - 1);

  if (isBase64Encoded) {
    result.data = decodeBase64(data);
    if (result.data.empty() && !data.empty()) {
      // base64 decode failed.
      return std::nullopt;
    }
  } else {
    result.data = std::vector<std::byte>(data.begin(), data.end());
  }

  return result;
}
} // namespace

void decodeDataUrls(
    const GltfReader& reader,
    GltfReaderResult& readGltf,
    const GltfReaderOptions& options) {
  CESIUM_TRACE("CesiumGltfReader::decodeDataUrls");
  if (!readGltf.model) {
    return;
  }

  CesiumGltf::Model& model = readGltf.model.value();

  for (CesiumGltf::Buffer& buffer : model.buffers) {
    if (!buffer.uri) {
      continue;
    }

    std::optional<DecodeResult> decoded = tryDecode(buffer.uri.value());
    if (!decoded) {
      continue;
    }

    buffer.cesium.data = std::move(decoded.value().data);

    if (options.clearDecodedDataUrls) {
      buffer.uri.reset();
    }
  }

  for (CesiumGltf::Image& image : model.images) {
    if (!image.uri) {
      continue;
    }

    std::optional<DecodeResult> decoded = tryDecode(image.uri.value());
    if (!decoded) {
      continue;
    }

    ImageReaderResult imageResult =
        reader.readImage(decoded.value().data, options.ktx2TranscodeTargets);
    if (imageResult.image) {
      image.cesium = std::move(imageResult.image.value());
    }

    if (options.clearDecodedDataUrls) {
      image.uri.reset();
    }
  }
}

} // namespace CesiumGltfReader
