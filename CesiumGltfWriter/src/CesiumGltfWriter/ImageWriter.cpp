#include "CesiumGltfWriter/ImageWriter.h"

#include "CesiumGltfWriter/Base64URIDetector.h"
#include "CesiumGltfWriter/EncodeBase64String.h"
#include "CesiumGltfWriter/ExtensionWriter.h"
#include "CesiumGltfWriter/WriteGLTFCallback.h"
#include "CesiumGltfWriter/WriteModelOptions.h"
#include "CesiumGltfWriter/WriteModelResult.h"

#include <CesiumGltf/Image.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <algorithm>
#include <cstdint>

namespace CesiumGltfWriter {

namespace {
[[nodiscard]] std::string
mimeTypeToExtensionString(const std::string& mimeType) noexcept {
  if (mimeType == CesiumGltf::Image::MimeType::image_jpeg) {
    return ".jpeg";
  }
  if (mimeType == CesiumGltf::Image::MimeType::image_png) {
    return ".png";
  }
  std::size_t slashIndex = mimeType.find('/');
  if (slashIndex != std::string::npos) {
    return "." + mimeType.substr(slashIndex + 1);
  }
  // TODO Print a warning here!
  return "";
}
} // namespace

void writeImage(
    WriteModelResult& result,
    const std::vector<CesiumGltf::Image>& images,
    CesiumJsonWriter::JsonWriter& jsonWriter,
    const WriteModelOptions& options,
    WriteGLTFCallback& writeGLTFCallback) {
  if (images.empty()) {
    return;
  }

  auto& j = jsonWriter;
  j.Key("images");
  j.StartArray();

  for (std::size_t i = 0; i < images.size(); ++i) {
    auto& image = images.at(i);
    const auto isUriSet = image.uri.has_value();
    const auto isDataBufferEmpty = image.cesium.pixelData.empty();
    const auto isBase64URI = isUriSet && isURIBase64DataURI(*image.uri);
    const auto isExternalFileURI = isUriSet && !isBase64URI;

    j.StartObject();

    if (isBase64URI) {
      if (!isDataBufferEmpty) {
        const std::string culpableImage = "images[" + std::to_string(i) + "]";
        std::string error = culpableImage + ".uri cannot be a base64 uri if " +
                            culpableImage + ".cesium.pixelData is non-empty";
        result.errors.push_back(std::move(error));
        j.EndObject();
        j.EndArray();
        return;
      }

      j.KeyPrimitive("uri", *image.uri);
    }

    else if (isExternalFileURI) {
      if (!isDataBufferEmpty) {
        const std::string culpableImage = "images[" + std::to_string(i) + "]";
        std::string error = culpableImage +
                            ".uri references an external file but " +
                            culpableImage + ".cesium.pixelData is empty";
        result.errors.push_back(std::move(error));
        j.EndObject();
        j.EndArray();
        return;
      }

      writeGLTFCallback(*image.uri, image.cesium.pixelData);
    }

    else if (!isDataBufferEmpty) {
      if (options.autoConvertDataToBase64) {
        j.KeyPrimitive(
            "uri",
            BASE64_PREFIX + encodeAsBase64String(image.cesium.pixelData));
      }

      // Automatically generate a uri using the mimetype / index and call
      // the user provided lambda.
      else {
        const auto ext =
            image.mimeType ? mimeTypeToExtensionString(*image.mimeType) : "";
        writeGLTFCallback(std::to_string(i) + ext, image.cesium.pixelData);
      }
    }

    if (image.mimeType) {
      j.Key("mimeType");
      j.String(*image.mimeType);
    }

    if (image.bufferView >= 0) {
      j.KeyPrimitive("bufferView", image.bufferView);
    }

    if (!image.name.empty()) {
      j.KeyPrimitive("name", image.name);
    }

    if (!image.extras.empty()) {
      j.Key("extras");
      writeJsonValue(image.extras, j);
    }

    if (!image.extensions.empty()) {
      writeExtensions(image.extensions, j);
    }

    j.EndObject();
  }

  j.EndArray();
}
} // namespace CesiumGltfWriter