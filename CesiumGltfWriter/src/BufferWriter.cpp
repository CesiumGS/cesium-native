#include "BufferWriter.h"
#include "Base64URIDetector.h"
#include "EncodeBase64String.h"
#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include <CesiumGltf/WriteModelOptions.h>
#include <string_view>

void CesiumGltf::writeBuffer(
    WriteModelResult& result,
    const std::vector<Buffer>& buffers,
    JsonWriter& jsonWriter,
    const WriteModelOptions& options,
    WriteGLTFCallback writeGLTFCallback) {
  auto& j = jsonWriter;

  if (buffers.empty()) {
    return;
  }

  j.Key("buffers");
  j.StartArray();

  for (std::size_t i = 0; i < buffers.size(); ++i) {
    auto& buffer = buffers.at(i);
    const auto isBufferReservedForGLBBinaryChunk =
        i == 0 && options.exportType == GltfExportType::GLB;
    const auto isUriSet = buffer.uri.has_value();
    const auto isDataBufferEmpty = buffer.cesium.data.empty();
    const auto isBase64URI = isUriSet && isURIBase64DataURI(*buffer.uri);
    const auto isExternalFileURI = isUriSet && !isBase64URI;

    j.StartObject();

    std::int64_t byteLength = buffer.byteLength;
    if (isBufferReservedForGLBBinaryChunk) {
      if (isUriSet) {
        const std::string culpableBuffer = "buffers[" + std::to_string(i) + "]";
        std::string error = "URIErroneouslyDefined: " + culpableBuffer + " " +
                            "should NOT be set in GLB mode " +
                            "(0th buffer is reserved)";
        result.errors.push_back(std::move(error));
        j.EndObject();
        j.EndArray();
        return;
      }

      byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
    }

    else if (isBase64URI) {
      if (!isDataBufferEmpty) {
        const std::string culpableBuffer = "buffers[" + std::to_string(i) + "]";
        std::string error = "AmbiguiousDataSource: " + culpableBuffer + " " +
                            "has a base64 data uri but " + culpableBuffer +
                            ".cesium.data should be empty if " +
                            culpableBuffer + ".uri is a base64 uri";
        result.errors.push_back(std::move(error));
        j.EndObject();
        j.EndArray();
        return;
      }

      if (byteLength == 0) {
        const std::string culpableBuffer = "buffers[" + std::to_string(i) + "]";
        std::string error = "ByteLengthNotSet: " + culpableBuffer +
                            ".uri is a " +
                            "base64 data uri, but buffer.byteLength is 0 " +
                            "(Empty base64 uri strings are not supported)";
        result.errors.push_back(std::move(error));
        j.EndObject();
        j.EndArray();
        return;
      }
      j.KeyPrimitive("uri", *buffer.uri);
    }

    else if (isExternalFileURI) {
      if (isDataBufferEmpty) {
        const std::string culpableBuffer = "buffers[" + std::to_string(i) + "]";
        std::string error = "MissingDataSource: " + culpableBuffer + ".uri " +
                            "is an external file uri, but " + culpableBuffer +
                            ".cesium.data is empty. " + culpableBuffer +
                            ".cesium.data must be non-empty if " +
                            culpableBuffer + ".uri is an external file uri";
        result.errors.emplace_back(std::move(error));
        j.EndObject();
        j.EndArray();
        return;
      }
      byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
      j.KeyPrimitive("uri", *buffer.uri);
      writeGLTFCallback(*buffer.uri, buffer.cesium.data);
    }

    else if (!isDataBufferEmpty) {
      if (options.autoConvertDataToBase64) {
        byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
        j.KeyPrimitive(
            "uri",
            BASE64_PREFIX + encodeAsBase64String(buffer.cesium.data));
      }

      // Auto generate a filename and invoke the user provided lambda.
      else {
        writeGLTFCallback(std::to_string(i) + ".bin", buffer.cesium.data);
      }
    }

    j.KeyPrimitive("byteLength", byteLength);

    if (!buffer.name.empty()) {
      j.KeyPrimitive("name", buffer.name);
    }

    if (!buffer.extras.empty()) {
      j.Key("extras");
      writeJsonValue(buffer.extras, j, false);
    }

    if (!buffer.extensions.empty()) {
      writeExtensions(buffer.extensions, j);
    }

    j.EndObject();
  }

  j.EndArray();
}
