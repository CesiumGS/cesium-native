#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include "BufferWriter.h"
#include <stdexcept>

// TODO: Add const std::vector<Buffer>&& buffers variant
//       that consumes the model, that way we can MOVE the buffers
//       into the amalgamated one instead of performing extra copies.
std::vector<std::uint8_t> CesiumGltf::writeBuffer(
    const std::vector<Buffer>& buffers,
    CesiumGltf::JsonWriter& jsonWriter) {
    auto& j = jsonWriter;
    std::vector<std::uint8_t> amalgamatedBuffer;

    if (buffers.empty()) {
        return amalgamatedBuffer;
    }

    j.Key("buffers");
    j.StartArray();

    for (const auto& buffer : buffers) {
        j.StartObject();
        if (buffer.uri) {
            // TODO: Check to see if the URI is a reference to an
            // external file. We need to return a struct of some sort
            // with a list of these.
            j.KeyPrimitive("uri", *buffer.uri);
        }

        auto& src = buffer.cesium.data;
        amalgamatedBuffer.insert(
            amalgamatedBuffer.end(),
            src.begin(),
            src.end());

        j.Key("byteLength");
        j.Int64(buffer.byteLength);

        if (!buffer.name.empty()) {
            j.Key("name");
            j.String(buffer.name.c_str());
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