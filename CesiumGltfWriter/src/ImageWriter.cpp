#include "ImageWriter.h"
#include "Base64URIDetector.h"
#include "EncodeBase64String.h"
#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include <CesiumGltf/Image.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <algorithm>
#include <cstdint>

[[nodiscard]] std::string
mimeTypeToMimeString(CesiumGltf::Image::MimeType mimeType) noexcept {
  if (mimeType == CesiumGltf::Image::MimeType::image_jpeg) {
    return "image/jpeg";
  } else {
    return "image/png";
  }
}

[[nodiscard]] std::string
mimeTypeToExtensionString(CesiumGltf::Image::MimeType mimeType) noexcept {
  if (mimeType == CesiumGltf::Image::MimeType::image_jpeg) {
    return ".jpeg";
  } else {
    return ".png";
  }
}

void CesiumGltf::writeImage(
    WriteModelResult& result,
    const std::vector<CesiumGltf::Image>& images,
    CesiumGltf::JsonWriter& jsonWriter,
    const WriteModelOptions& options,
    WriteGLTFCallback writeGLTFCallback) {
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
      j.String(mimeTypeToMimeString(*image.mimeType));
    }

    if (image.bufferView >= 0) {
      j.KeyPrimitive("bufferView", image.bufferView);
    }

    if (!image.name.empty()) {
      j.KeyPrimitive("name", image.name);
    }

    if (!image.extras.empty()) {
      j.Key("extras");
      writeJsonValue(image.extras, j, false);
    }

    if (!image.extensions.empty()) {
      writeExtensions(image.extensions, j);
    }

    j.EndObject();
  }

  j.EndArray();
}
