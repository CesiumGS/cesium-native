#include "ModelJsonWriter.h"
#include "registerWriterExtensions.h"

#include <CesiumGltfWriter/GltfWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Tracing.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <vector>

namespace CesiumGltfWriter {

namespace {

[[nodiscard]] size_t
getPadding(size_t byteCount, size_t byteAlignment) noexcept {
  CESIUM_ASSERT(byteAlignment > 0);
  size_t remainder = byteCount % byteAlignment;
  size_t padding = remainder == 0 ? 0 : byteAlignment - remainder;
  return padding;
}

void writeGlbBuffer(
    GltfWriterResult& result,
    const std::span<const std::byte>& jsonData,
    const std::span<const std::byte>& bufferData,
    size_t binaryChunkByteAlignment) {
  CESIUM_ASSERT(
      binaryChunkByteAlignment > 0 && binaryChunkByteAlignment % 4 == 0);

  size_t headerSize = 12;
  size_t chunkHeaderSize = 8;

  size_t jsonPaddingSize =
      getPadding(headerSize + chunkHeaderSize + jsonData.size(), 4);
  size_t jsonChunkDataSize = jsonData.size() + jsonPaddingSize;
  size_t glbSize = headerSize + chunkHeaderSize + jsonChunkDataSize;

  size_t binaryPaddingSize = 0;
  size_t binaryChunkDataSize = 0;

  if (bufferData.size() > 0) {
    size_t extraJsonPadding =
        getPadding(glbSize + chunkHeaderSize, binaryChunkByteAlignment);
    if (extraJsonPadding > 0) {
      jsonPaddingSize += extraJsonPadding;
      jsonChunkDataSize += extraJsonPadding;
      glbSize += extraJsonPadding;
    }

    binaryPaddingSize =
        getPadding(glbSize + chunkHeaderSize + bufferData.size(), 4);
    binaryChunkDataSize = bufferData.size() + binaryPaddingSize;
    glbSize += chunkHeaderSize + binaryChunkDataSize;
  }

  // GLB stores its own length as a uint32. So if that would be >= 4GB , we
  // can't output a valid GLB.
  if (glbSize > size_t(std::numeric_limits<uint32_t>::max())) {
    result.errors.emplace_back(
        "glTF is too large to represent as a binary glTF (GLB). The total size "
        "of the GLB must be less than 4GB.");
    return;
  }

  std::vector<std::byte>& glb = result.gltfBytes;
  glb.resize(glbSize);
  uint8_t* glb8 = reinterpret_cast<uint8_t*>(glb.data());
  uint32_t* glb32 = reinterpret_cast<uint32_t*>(glb.data());

  // GLB header
  size_t byteOffset = 0;
  glb8[byteOffset++] = 'g';
  glb8[byteOffset++] = 'l';
  glb8[byteOffset++] = 'T';
  glb8[byteOffset++] = 'F';
  glb32[byteOffset / 4] = 2;
  byteOffset += 4;
  glb32[byteOffset / 4] = static_cast<uint32_t>(glbSize);
  byteOffset += 4;

  // JSON chunk header
  glb32[byteOffset / 4] = static_cast<uint32_t>(jsonChunkDataSize);
  byteOffset += 4;
  glb8[byteOffset++] = 'J';
  glb8[byteOffset++] = 'S';
  glb8[byteOffset++] = 'O';
  glb8[byteOffset++] = 'N';

  // JSON chunk
  memcpy(glb8 + byteOffset, jsonData.data(), jsonData.size());
  byteOffset += jsonData.size();

  // JSON chunk padding
  memset(glb8 + byteOffset, ' ', jsonPaddingSize);
  byteOffset += jsonPaddingSize;

  if (bufferData.size() > 0) {
    // Binary chunk header
    glb32[byteOffset / 4] = static_cast<uint32_t>(binaryChunkDataSize);
    byteOffset += 4;
    glb8[byteOffset++] = 'B';
    glb8[byteOffset++] = 'I';
    glb8[byteOffset++] = 'N';
    glb8[byteOffset++] = 0;

    // Binary chunk
    memcpy(glb8 + byteOffset, bufferData.data(), bufferData.size());
    byteOffset += bufferData.size();

    // Binary chunk padding
    memset(glb8 + byteOffset, 0, binaryPaddingSize);
  }
}
} // namespace

GltfWriter::GltfWriter() { registerWriterExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& GltfWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
GltfWriter::getExtensions() const {
  return this->_context;
}

GltfWriterResult GltfWriter::writeGltf(
    const CesiumGltf::Model& model,
    const GltfWriterOptions& options) const {
  CESIUM_TRACE("GltfWriter::writeGltf");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  GltfWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  ModelJsonWriter::write(model, *writer, context);
  result.gltfBytes = writer->toBytes();
  result.errors = writer->getErrors();
  result.warnings = writer->getWarnings();

  return result;
}

GltfWriterResult GltfWriter::writeGlb(
    const CesiumGltf::Model& model,
    const std::span<const std::byte>& bufferData,
    const GltfWriterOptions& options) const {
  CESIUM_TRACE("GltfWriter::writeGlb");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  GltfWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  ModelJsonWriter::write(model, *writer, context);
  std::vector<std::byte> jsonData = writer->toBytes();

  writeGlbBuffer(
      result,
      std::span(jsonData),
      bufferData,
      options.binaryChunkByteAlignment);

  result.errors.insert(
      result.errors.end(),
      writer->getErrors().begin(),
      writer->getErrors().end());

  result.warnings.insert(
      result.warnings.end(),
      writer->getWarnings().begin(),
      writer->getWarnings().end());

  return result;
}

} // namespace CesiumGltfWriter
