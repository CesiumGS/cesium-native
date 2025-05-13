#include "TilesetJsonWriter.h"
#include "registerWriterExtensions.h"

#include <Cesium3DTilesWriter/SubtreeWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Tracing.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

namespace Cesium3DTilesWriter {

namespace {

[[nodiscard]] size_t
getPadding(size_t byteCount, size_t byteAlignment) noexcept {
  CESIUM_ASSERT(byteAlignment > 0);
  size_t remainder = byteCount % byteAlignment;
  size_t padding = remainder == 0 ? 0 : byteAlignment - remainder;
  return padding;
}

void writeSubtreeBuffer(
    SubtreeWriterResult& result,
    const std::span<const std::byte>& jsonData,
    const std::span<const std::byte>& bufferData) {
  size_t byteAlignment = 8;
  size_t headerSize = 24;

  size_t jsonPaddingSize =
      getPadding(headerSize + jsonData.size(), byteAlignment);
  size_t jsonDataSize = jsonData.size() + jsonPaddingSize;
  size_t subtreeSize = headerSize + jsonDataSize;

  size_t binaryPaddingSize = 0;
  size_t binaryDataSize = 0;

  if (bufferData.size() > 0) {
    binaryPaddingSize =
        getPadding(subtreeSize + bufferData.size(), byteAlignment);
    binaryDataSize = bufferData.size() + binaryPaddingSize;
    subtreeSize += binaryDataSize;
  }

  std::vector<std::byte>& subtree = result.subtreeBytes;
  subtree.resize(subtreeSize);
  uint8_t* subtree8 = reinterpret_cast<uint8_t*>(subtree.data());
  uint32_t* subtree32 = reinterpret_cast<uint32_t*>(subtree.data());
  uint64_t* subtree64 = reinterpret_cast<uint64_t*>(subtree.data());

  // Subtree header
  size_t byteOffset = 0;
  subtree8[byteOffset++] = 's';
  subtree8[byteOffset++] = 'u';
  subtree8[byteOffset++] = 'b';
  subtree8[byteOffset++] = 't';
  subtree32[byteOffset / 4] = 1;
  byteOffset += 4;
  subtree64[byteOffset / 8] = static_cast<uint64_t>(jsonDataSize);
  byteOffset += 8;
  subtree64[byteOffset / 8] = static_cast<uint64_t>(binaryDataSize);
  byteOffset += 8;

  // JSON section
  memcpy(subtree8 + byteOffset, jsonData.data(), jsonData.size());
  byteOffset += jsonData.size();

  // JSON section padding
  memset(subtree8 + byteOffset, ' ', jsonPaddingSize);
  byteOffset += jsonPaddingSize;

  if (bufferData.size() > 0) {
    // Binary section
    memcpy(subtree8 + byteOffset, bufferData.data(), bufferData.size());
    byteOffset += bufferData.size();

    // Binary section padding
    memset(subtree8 + byteOffset, 0, binaryPaddingSize);
  }
}

} // namespace

SubtreeWriter::SubtreeWriter() { registerWriterExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& SubtreeWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
SubtreeWriter::getExtensions() const {
  return this->_context;
}

SubtreeWriterResult SubtreeWriter::writeSubtreeJson(
    const Cesium3DTiles::Subtree& subtree,
    const SubtreeWriterOptions& options) const {
  CESIUM_TRACE("SubtreeWriter::writeSubtreeJson");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  SubtreeWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  SubtreeJsonWriter::write(subtree, *writer, context);
  result.subtreeBytes = writer->toBytes();
  result.errors = writer->getErrors();
  result.warnings = writer->getWarnings();

  return result;
}

SubtreeWriterResult SubtreeWriter::writeSubtreeBinary(
    const Cesium3DTiles::Subtree& subtree,
    const std::span<const std::byte>& bufferData,
    const SubtreeWriterOptions& options) const {
  CESIUM_TRACE("SubtreeWriter::writeSubtreeBinary");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  SubtreeWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  SubtreeJsonWriter::write(subtree, *writer, context);
  std::vector<std::byte> jsonData = writer->toBytes();

  writeSubtreeBuffer(result, std::span(jsonData), bufferData);

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

} // namespace Cesium3DTilesWriter
