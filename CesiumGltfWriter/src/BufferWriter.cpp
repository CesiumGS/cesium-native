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

std::vector<std::uint8_t> CesiumGltf::writeBuffer(
    const std::vector<Buffer>& buffers,
    JsonWriter& jsonWriter,
    WriteFlags flags,
    WriteGLTFCallback writeGLTFCallback
) {
    auto& j = jsonWriter;
    std::vector<std::uint8_t> amalgamatedBuffer;

    if (buffers.empty()) {
        return amalgamatedBuffer;
    }

    j.Key("buffers");
    j.StartArray();
    
    for (auto i = 0ul; i < buffers.size(); ++i) {
        auto& buffer = buffers.at(i);
        const auto bufferReservedForGLB = i == 0 && flags & WriteFlags::GLB;
        const auto uriSet = buffer.uri.has_value();
        const auto dataBufferNonEmpty = !buffer.cesium.data.empty();
        const auto isBase64URI = uriSet && isURIBase64DataURI(*buffer.uri);
        const auto isExternalFileURI = uriSet && !isBase64URI;

        j.StartObject();

        std::int64_t byteLength = buffer.byteLength;
        if (bufferReservedForGLB) {
            if (uriSet) {
                throw URIErroneouslyDefined("model.buffers[0].uri should NOT be set in GLB mode. (0th buffer is reserved)");
            }
            
            byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
        } 

        else if (isBase64URI && dataBufferNonEmpty) {
            if (buffer.byteLength == 0) {
                throw std::runtime_error("buffer.uri has base64 data uri, but no buffer.byteLength");
            }
            j.KeyPrimitive("uri", *buffer.uri);
        }
        
        else if (isExternalFileURI && dataBufferNonEmpty) {
            byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
            writeGLTFCallback(*buffer.uri, buffer.cesium.data);
        }
        
        else if (!uriSet && dataBufferNonEmpty) {
            if (flags & WriteFlags::AutoConvertDataToBase64) {
                byteLength = static_cast<std::int64_t>(buffer.cesium.data.size());
                j.KeyPrimitive("uri", BASE64_PREFIX + encodeAsBase64String(buffer.cesium.data));
            } else {
                writeGLTFCallback(std::to_string(i) + ".bin", buffer.cesium.data);
            }
        }
        
        else if (dataBufferNonEmpty && !buffer.uri) {
            // TODO: Better exception message, custom exception
            throw std::runtime_error("buffer.cesium.data was empty, but buffer.uri was set");
        }

        j.KeyPrimitive("byteLength", byteLength);

        if (!buffer.name.empty()) {
            j.KeyPrimitive("name", buffer.name);
        }

        if (!buffer.extras.empty()) {
            j.Key("extras");
            writeJsonValue(buffer.extras, j);
        }

        if (!buffer.extensions.empty()) {
            writeExtensions(buffer.extensions, j);
        }

        j.EndObject();
    }

    j.EndArray();
    return amalgamatedBuffer;
}