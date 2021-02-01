#include "GLBWriter.h"
#include "AccessorWriter.h"
#include <rapidjson/fwd.h>

namespace CesiumGltf {
    GLBWriter::GLBWriter(const Model model) : _model(std::move(model)) { parse(); }

    std::vector<uint8_t> GLBWriter::toGLBByteArray() const {
        std::vector<uint8_t> result;
        return result;
    }

    void GLBWriter::writeGLBToDisk(
        const std::filesystem::path& /*outputPath*/) {}

    void GLBWriter::parse() {
        writeAccessor(_model.accessors, _jsonWriter);
    }

}