#include "WriteBinaryGLB.h"
#include <algorithm>
#include <magic_enum.hpp>

const std::size_t BYTE_HEADER_SIZE = 12;
const std::size_t CHUNK_HEADER_MINIMUM_SIZE = 8;
const std::uint32_t GLTF_VERSION = 2;
const std::uint8_t PADDING_CHAR = 0x20;

[[nodiscard]] inline std::size_t nextMultipleOfFour(std::size_t n) noexcept {
    return (n + 3) & ~0x03ul;
}

void writeGLTFHeader(std::vector<std::uint8_t>& glbBuffer) noexcept {
    glbBuffer[0] = 'g';
    glbBuffer[1] = 'l';
    glbBuffer[2] = 'T';
    glbBuffer[3] = 'F';

    glbBuffer[4] = GLTF_VERSION & 0xff;
    glbBuffer[5] = (GLTF_VERSION >> 8) & 0xff;
    glbBuffer[6] = (GLTF_VERSION >> 16) & 0xff;
    glbBuffer[7] = (GLTF_VERSION >> 24) & 0xff;
}

void writeGLBSize(std::vector<std::uint8_t>& glbBuffer) noexcept {
    const auto totalGLBLength = glbBuffer.size();
    glbBuffer[8] = totalGLBLength & 0xff;
    glbBuffer[9] = (totalGLBLength >> 8) & 0xff;
    glbBuffer[10] = (totalGLBLength >> 16) & 0xff;
    glbBuffer[11] = (totalGLBLength >> 24) & 0xff;
}

void writeGLBJsonChunk(
    std::vector<std::uint8_t>& glbBuffer,
    const std::string_view& gltfJson) {
    const auto jsonLength = gltfJson.size();
    const auto paddingLength = nextMultipleOfFour(jsonLength) - jsonLength;
    const auto chunkLength = jsonLength + paddingLength;

    glbBuffer[12] = chunkLength & 0xff;
    glbBuffer[13] = (chunkLength >> 8) & 0xff;
    glbBuffer[14] = (chunkLength >> 16) & 0xff;
    glbBuffer[15] = (chunkLength >> 24) & 0xff;

    glbBuffer[16] = GLBChunkType::JSON & 0xff;
    glbBuffer[17] = (GLBChunkType::JSON >> 8) & 0xff;
    glbBuffer[18] = (GLBChunkType::JSON >> 16) & 0xff;
    glbBuffer[19] = (GLBChunkType::JSON >> 24) & 0xff;

    glbBuffer.insert(glbBuffer.end(), gltfJson.begin(), gltfJson.end());
    glbBuffer.insert(glbBuffer.end(), paddingLength, PADDING_CHAR);
}

void writeGLBBinaryChunk(
    std::vector<std::uint8_t>& glbBuffer,
    const std::vector<std::uint8_t>& binaryChunk,
    std::size_t byteOffset) {

    const auto bufferLength = binaryChunk.size();
    const auto paddingLength = nextMultipleOfFour(bufferLength) - bufferLength;
    const auto chunkLength = bufferLength + paddingLength;

    glbBuffer.resize(glbBuffer.size() + CHUNK_HEADER_MINIMUM_SIZE);

    glbBuffer[byteOffset + 0] = chunkLength & 0xff;
    glbBuffer[byteOffset + 1] = (chunkLength >> 8) & 0xff;
    glbBuffer[byteOffset + 2] = (chunkLength >> 16) & 0xff;
    glbBuffer[byteOffset + 3] = (chunkLength >> 24) & 0xff;

    glbBuffer[byteOffset + 4] = GLBChunkType::BIN & 0xff;
    glbBuffer[byteOffset + 5] = (GLBChunkType::BIN >> 8) & 0xff;
    glbBuffer[byteOffset + 6] = (GLBChunkType::BIN >> 16) & 0xff;
    glbBuffer[byteOffset + 7] = (GLBChunkType::BIN >> 24) & 0xff;

    glbBuffer.insert(glbBuffer.end(), binaryChunk.begin(), binaryChunk.end());
    glbBuffer.insert(glbBuffer.end(), paddingLength, PADDING_CHAR);
}

[[nodiscard]] std::vector<std::uint8_t> CesiumGltf::writeBinaryGLB(
    const std::vector<std::uint8_t>& binaryChunk,
    const std::string_view& gltfJson) {

    std::vector<std::uint8_t> glbBuffer(
        BYTE_HEADER_SIZE + CHUNK_HEADER_MINIMUM_SIZE);
    writeGLTFHeader(glbBuffer);
    writeGLBJsonChunk(glbBuffer, gltfJson);
    
    if (!binaryChunk.empty()) {
        writeGLBBinaryChunk(glbBuffer, binaryChunk, glbBuffer.size());
    }
    
    writeGLBSize(glbBuffer);
    return glbBuffer;
}