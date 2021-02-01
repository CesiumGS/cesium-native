#include "BufferWriter.h"
#include <stdexcept>

// TODO: Add const std::vector<Buffer>&& buffers variant
//       that consumes the model, that way we can MOVE the buffers
//       into the amalgamated one instead of performing extra copies.
std::vector<std::uint8_t> CesiumGltf::writeBuffer(
    const std::vector<Buffer>& buffers,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
    auto& j = jsonWriter;
    std::vector<std::uint8_t> amalgamatedBuffer;

    if (buffers.empty()) {
        return amalgamatedBuffer;
    }

    j.Key("buffers");
    j.StartArray();

    for (const auto& buffer : buffers) {

        if (buffer.cesium.data.empty() || buffer.uri) {
            throw std::runtime_error("External binary / embedded base64 uri "
                                     "currently not supported");
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

        // TODO extensions / extras
    }

    j.EndArray();
    return amalgamatedBuffer;
}