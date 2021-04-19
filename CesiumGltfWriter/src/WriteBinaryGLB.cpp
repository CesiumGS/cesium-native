#include "WriteBinaryGLB.h"
#include <algorithm>

const std::size_t BYTE_HEADER_SIZE = 12;
const std::size_t CHUNK_HEADER_MINIMUM_SIZE = 8;
const std::uint32_t GLB_CONTAINER_VERSION = 2;
const std::byte PADDING_CHAR = std::byte(0x20);

[[nodiscard]] inline std::size_t nextMultipleOfFour(std::size_t n) noexcept {
  return (n + 3) & ~0x03ul;
}

void writeGLTFHeader(std::vector<std::byte>& glbBuffer) noexcept {
  glbBuffer[0] = std::byte('g');
  glbBuffer[1] = std::byte('l');
  glbBuffer[2] = std::byte('T');
  glbBuffer[3] = std::byte('F');

  glbBuffer[4] = std::byte(GLB_CONTAINER_VERSION & 0xff);
  glbBuffer[5] = std::byte((GLB_CONTAINER_VERSION >> 8) & 0xff);
  glbBuffer[6] = std::byte((GLB_CONTAINER_VERSION >> 16) & 0xff);
  glbBuffer[7] = std::byte((GLB_CONTAINER_VERSION >> 24) & 0xff);
}

void writeGLBSize(std::vector<std::byte>& glbBuffer) noexcept {
  const auto totalGLBLength = glbBuffer.size();
  glbBuffer[8] = std::byte(totalGLBLength & 0xff);
  glbBuffer[9] = std::byte((totalGLBLength >> 8) & 0xff);
  glbBuffer[10] = std::byte((totalGLBLength >> 16) & 0xff);
  glbBuffer[11] = std::byte((totalGLBLength >> 24) & 0xff);
}

void writeGLBJsonChunk(
    std::vector<std::byte>& glbBuffer,
    const std::string_view& gltfJson) {
  const auto jsonLength = gltfJson.size();
  const auto paddingLength = nextMultipleOfFour(jsonLength) - jsonLength;
  const auto chunkLength = jsonLength + paddingLength;

  glbBuffer[12] = std::byte(chunkLength & 0xff);
  glbBuffer[13] = std::byte((chunkLength >> 8) & 0xff);
  glbBuffer[14] = std::byte((chunkLength >> 16) & 0xff);
  glbBuffer[15] = std::byte((chunkLength >> 24) & 0xff);

  glbBuffer[16] = std::byte(GLBChunkType::JSON & 0xff);
  glbBuffer[17] = std::byte((GLBChunkType::JSON >> 8) & 0xff);
  glbBuffer[18] = std::byte((GLBChunkType::JSON >> 16) & 0xff);
  glbBuffer[19] = std::byte((GLBChunkType::JSON >> 24) & 0xff);

  for (const auto c : gltfJson) {
    glbBuffer.emplace_back(std::byte(c));
  }

  glbBuffer.insert(glbBuffer.end(), paddingLength, PADDING_CHAR);
}

void writeGLBBinaryChunk(
    std::vector<std::byte>& glbBuffer,
    const std::vector<std::byte>& binaryChunk,
    std::size_t byteOffset) {

  const auto bufferLength = binaryChunk.size();
  const auto paddingLength = nextMultipleOfFour(bufferLength) - bufferLength;
  const auto chunkLength = bufferLength + paddingLength;

  glbBuffer.resize(glbBuffer.size() + CHUNK_HEADER_MINIMUM_SIZE);

  glbBuffer[byteOffset + 0] = std::byte(chunkLength & 0xff);
  glbBuffer[byteOffset + 1] = std::byte((chunkLength >> 8) & 0xff);
  glbBuffer[byteOffset + 2] = std::byte((chunkLength >> 16) & 0xff);
  glbBuffer[byteOffset + 3] = std::byte((chunkLength >> 24) & 0xff);

  glbBuffer[byteOffset + 4] = std::byte(GLBChunkType::BIN & 0xff);
  glbBuffer[byteOffset + 5] = std::byte((GLBChunkType::BIN >> 8) & 0xff);
  glbBuffer[byteOffset + 6] = std::byte((GLBChunkType::BIN >> 16) & 0xff);
  glbBuffer[byteOffset + 7] = std::byte((GLBChunkType::BIN >> 24) & 0xff);

  glbBuffer.insert(glbBuffer.end(), binaryChunk.begin(), binaryChunk.end());
  glbBuffer.insert(glbBuffer.end(), paddingLength, PADDING_CHAR);
}

[[nodiscard]] std::vector<std::byte> CesiumGltf::writeBinaryGLB(
    const std::vector<std::byte>& binaryChunk,
    const std::string_view& gltfJson) {

  std::vector<std::byte> glbBuffer(
      BYTE_HEADER_SIZE + CHUNK_HEADER_MINIMUM_SIZE);
  writeGLTFHeader(glbBuffer);
  writeGLBJsonChunk(glbBuffer, gltfJson);

  if (!binaryChunk.empty()) {
    writeGLBBinaryChunk(glbBuffer, binaryChunk, glbBuffer.size());
  }

  writeGLBSize(glbBuffer);
  return glbBuffer;
}
