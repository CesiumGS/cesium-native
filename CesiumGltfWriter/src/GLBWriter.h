#pragma once
#include "CesiumGltf/Model.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <cstdint>
#include <filesystem>

namespace CesiumGltf {
    struct GLBWriter {
        GLBWriter(const Model model);
        GLBWriter(const GLBWriter&) = delete;
        GLBWriter& operator=(const GLBWriter&) = delete;

        std::vector<uint8_t> toGLBByteArray() const;
        void writeGLBToDisk(const std::filesystem::path& outputPath);
    private:
        Model _model;
        rapidjson::Writer<rapidjson::StringBuffer> _jsonWriter;
        std::vector<std::vector<uint8_t>> _littleEndianBuffers;

        void parse();
    };
}