#include "decodeDataUrls.h"

#include "CesiumGltf/GltfReader.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ReaderContext.h"
#include "CesiumUtility/Tracing.h"

#include <modp_b64.h>

#include <cstddef>

namespace {

std::vector<std::byte> decodeBase64(gsl::span<const std::byte> data) {
  CESIUM_TRACE("CesiumGltf::decodeBase64");
  std::vector<std::byte> result(modp_b64_decode_len(data.size()));

  size_t resultLength = modp_b64_decode(
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

  size_t dataDelimeter = uri.find(',', dataPrefixLength);
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

  gsl::span<const std::byte> data(
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

namespace CesiumGltf {

void decodeDataUrls(
    const ReaderContext& context,
    ModelReaderResult& readModel,
    bool clearDecodedDataUrls) {
  CESIUM_TRACE("CesiumGltf::decodeDataUrls");
  if (!readModel.model) {
    return;
  }

  Model& model = readModel.model.value();

  for (Buffer& buffer : model.buffers) {
    if (!buffer.uri) {
      continue;
    }

    std::optional<DecodeResult> decoded = tryDecode(buffer.uri.value());
    if (!decoded) {
      continue;
    }

    buffer.cesium.data = std::move(decoded.value().data);

    if (clearDecodedDataUrls) {
      buffer.uri.reset();
    }
  }

  for (Image& image : model.images) {
    if (!image.uri) {
      continue;
    }

    std::optional<DecodeResult> decoded = tryDecode(image.uri.value());
    if (!decoded) {
      continue;
    }

    ImageReaderResult imageResult =
        context.reader.readImage(decoded.value().data);
    if (imageResult.image) {
      image.cesium = std::move(imageResult.image.value());
    }

    if (clearDecodedDataUrls) {
      image.uri.reset();
    }
  }
}

} // namespace CesiumGltf
