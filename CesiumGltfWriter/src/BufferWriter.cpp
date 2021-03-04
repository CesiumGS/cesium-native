#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include "BufferWriter.h"
#include "EncodeBase64String.h"
#include <CesiumGltf/WriterException.h>
#include <string_view>

constexpr auto BASE64_PREFIX = "data:application/octet-stream;base64,";

[[nodiscard]] inline bool isURIBase64DataURI(std::string_view view) noexcept {
    return view.rfind(BASE64_PREFIX, 0) == 0;
}

void CesiumGltf::writeBuffer(
    const std::vector<Buffer>& buffers,
    JsonWriter& jsonWriter,
    WriteFlags flags,
    WriteGLTFCallback writeGLTFCallback
) {
    auto& j = jsonWriter;

    if (buffers.empty()) {
        return;
    }

    j.Key("buffers");
    j.StartArray();
    
    for (auto i = 0ul; i < buffers.size(); ++i) {
        auto& buffer = buffers.at(i);
        const auto isBufferReservedForGLBBinaryChunk = i == 0 && flags & WriteFlags::GLB;
        const auto isUriSet = buffer.uri.has_value();
        const auto isDataBufferEmpty = buffer.cesium.data.empty();
        const auto isBase64URI = isUriSet && isURIBase64DataURI(*buffer.uri);
        const auto isExternalFileURI = isUriSet && !isBase64URI;

        j.StartObject();

        std::int64_t byteLength = buffer.byteLength;
        if (isBufferReservedForGLBBinaryChunk) {
            if (isUriSet) {
                throw URIErroneouslyDefined("model.buffers[0].uri should NOT be set in GLB mode. (0th buffer is reserved)");
            }
            
            byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
        } 

        else if (isBase64URI) {
            if (!isDataBufferEmpty) {
                throw AmbiguiousDataSource("buffer.uri is base64 data uri, but buffer.cesium.data is non-empty. Buffer.cesium.data should be empty if buffer.uri is a base64 uri");
            }

            if (byteLength == 0) {
                throw ByteLengthNotSet("buffer.uri is base64 data uri, but buffer.byteLength is 0. (Empty base64 uri strings not supported)");
            }
            j.KeyPrimitive("uri", *buffer.uri);
        }
        
        else if (isExternalFileURI) {
            if (isDataBufferEmpty) {
                throw MissingDataSource("buffer.uri is external file uri, but buffer.cesium.data is empty. Buffer.cesium.data must be non empty if uri is external file uri");
            }
            byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
            j.KeyPrimitive("uri", *buffer.uri);
            writeGLTFCallback(*buffer.uri, buffer.cesium.data);
        }

        else if (!isDataBufferEmpty) {
            if (flags & WriteFlags::AutoConvertDataToBase64) {
                byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
                j.KeyPrimitive("uri", BASE64_PREFIX + encodeAsBase64String(buffer.cesium.data));
            } 

            // Default to calling the user provided lambda if they don't opt into auto
            // base64 conversion.
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