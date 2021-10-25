#include "CesiumGltf/GltfReader.h"

#include "ExtensionKhrDracoMeshCompressionJsonHandler.h"
#include "ExtensionMeshPrimitiveExtFeatureMetadataJsonHandler.h"
#include "ExtensionModelExtFeatureMetadataJsonHandler.h"
#include "ModelJsonHandler.h"
#include "decodeDataUrls.h"
#include "decodeDraco.h"

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

#include <rapidjson/reader.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>

using namespace CesiumGltf;
using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace {

#pragma pack(push, 1)
struct GlbHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t length;
};

struct ChunkHeader {
  uint32_t chunkLength;
  uint32_t chunkType;
};
#pragma pack(pop)

bool isBinaryGltf(const gsl::span<const std::byte>& data) noexcept {
  if (data.size() < sizeof(GlbHeader)) {
    return false;
  }

  return reinterpret_cast<const GlbHeader*>(data.data())->magic == 0x46546C67;
}

ModelReaderResult readJsonModel(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("CesiumGltf::ModelReader::readJsonModel");

  ModelJsonHandler modelHandler(context);
  ReadJsonResult<Model> jsonResult = JsonReader::readJson(data, modelHandler);

  return ModelReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

namespace {
/**
 * @brief Creates a string representation for the given magic value.
 *
 * The details are not specified, but the output will include a
 * hex representation of the given value, as well as the result
 * of interpreting the value as 4 unsigned characters.
 *
 * @param i The value
 * @return The string
 */
std::string toMagicString(uint32_t i) {
  const unsigned char c0 = static_cast<unsigned char>(i & 0xFF);
  const unsigned char c1 = static_cast<unsigned char>((i >> 8) & 0xFF);
  const unsigned char c2 = static_cast<unsigned char>((i >> 16) & 0xFF);
  const unsigned char c3 = static_cast<unsigned char>((i >> 24) & 0xFF);
  std::stringstream stream;
  stream << c0 << c1 << c2 << c3 << " (0x" << std::hex << i << ")";
  return stream.str();
}
} // namespace

ModelReaderResult readBinaryModel(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {
  CESIUM_TRACE("CesiumGltf::ModelReader::readBinaryModel");

  if (data.size() < sizeof(GlbHeader) + sizeof(ChunkHeader)) {
    return {std::nullopt, {"Too short to be a valid GLB."}, {}};
  }

  const GlbHeader* pHeader = reinterpret_cast<const GlbHeader*>(data.data());
  if (pHeader->magic != 0x46546C67) {
    return {
        std::nullopt,
        {"GLB does not start with the expected magic value 'glTF', but " +
         toMagicString(pHeader->magic)},
        {}};
  }

  if (pHeader->version != 2) {
    return {
        std::nullopt,
        {"Only binary glTF version 2 is supported, found version " +
         std::to_string(pHeader->version)},
        {}};
  }

  if (pHeader->length > data.size()) {
    return {
        std::nullopt,
        {"GLB extends past the end of the buffer, header size " +
         std::to_string(pHeader->length) + ", data size " +
         std::to_string(data.size())},
        {}};
  }

  const gsl::span<const std::byte> glbData = data.subspan(0, pHeader->length);

  const ChunkHeader* pJsonChunkHeader =
      reinterpret_cast<const ChunkHeader*>(glbData.data() + sizeof(GlbHeader));
  if (pJsonChunkHeader->chunkType != 0x4E4F534A) {
    return {
        std::nullopt,
        {"GLB JSON chunk does not have the expected chunkType 'JSON', but " +
         toMagicString(pJsonChunkHeader->chunkType)},
        {}};
  }

  const size_t jsonStart = sizeof(GlbHeader) + sizeof(ChunkHeader);
  const size_t jsonEnd = jsonStart + pJsonChunkHeader->chunkLength;

  if (jsonEnd > glbData.size()) {
    return {
        std::nullopt,
        {"GLB JSON chunk extends past the end of the buffer, JSON end at " +
         std::to_string(jsonEnd) + ", data size " +
         std::to_string(glbData.size())},
        {}};
  }

  const gsl::span<const std::byte> jsonChunk =
      glbData.subspan(jsonStart, pJsonChunkHeader->chunkLength);
  gsl::span<const std::byte> binaryChunk;

  if (jsonEnd + sizeof(ChunkHeader) <= data.size()) {
    const ChunkHeader* pBinaryChunkHeader =
        reinterpret_cast<const ChunkHeader*>(glbData.data() + jsonEnd);
    if (pBinaryChunkHeader->chunkType != 0x004E4942) {
      return {
          std::nullopt,
          {"GLB binary chunk does not have the expected chunkType 'BIN', but " +
           toMagicString(pBinaryChunkHeader->chunkType)},
          {}};
    }

    const size_t binaryStart = jsonEnd + sizeof(ChunkHeader);
    const size_t binaryEnd = binaryStart + pBinaryChunkHeader->chunkLength;

    if (binaryEnd > glbData.size()) {
      return {
          std::nullopt,
          {"GLB binary chunk extends past the end of the buffer, binary end "
           "at " +
           std::to_string(binaryEnd) + ", data size " +
           std::to_string(glbData.size())},
          {}};
    }

    binaryChunk = glbData.subspan(binaryStart, pBinaryChunkHeader->chunkLength);
  }

  ModelReaderResult result = readJsonModel(context, jsonChunk);

  if (result.model && !binaryChunk.empty()) {
    Model& model = result.model.value();

    if (model.buffers.empty()) {
      result.errors.emplace_back(
          "GLB has a binary chunk but the JSON does not define any buffers.");
      return result;
    }

    Buffer& buffer = model.buffers[0];
    if (buffer.uri) {
      result.errors.emplace_back("GLB has a binary chunk but the first buffer "
                                 "in the JSON chunk also has a 'uri'.");
      return result;
    }

    const int64_t binaryChunkSize = static_cast<int64_t>(binaryChunk.size());
    if (buffer.byteLength > binaryChunkSize ||
        buffer.byteLength + 3 < binaryChunkSize) {
      result.errors.emplace_back("GLB binary chunk size does not match the "
                                 "size of the first buffer in the JSON chunk.");
      return result;
    }

    buffer.cesium.data = std::vector<std::byte>(
        binaryChunk.begin(),
        binaryChunk.begin() + buffer.byteLength);
  }

  return result;
}

void postprocess(
    const GltfReader& reader,
    ModelReaderResult& readModel,
    const ReadModelOptions& options) {
  Model& model = readModel.model.value();

  if (options.decodeDataUrls) {
    decodeDataUrls(reader, readModel, options.clearDecodedDataUrls);
  }

  if (options.decodeEmbeddedImages) {
    CESIUM_TRACE("CesiumGltf::decodeEmbeddedImages");
    for (Image& image : model.images) {
      const BufferView& bufferView =
          Model::getSafe(model.bufferViews, image.bufferView);
      const Buffer& buffer = Model::getSafe(model.buffers, bufferView.buffer);

      if (bufferView.byteOffset + bufferView.byteLength >
          static_cast<int64_t>(buffer.cesium.data.size())) {
        readModel.warnings.emplace_back(
            "Image bufferView's byte offset is " +
            std::to_string(bufferView.byteOffset) + " and the byteLength is " +
            std::to_string(bufferView.byteLength) + ", the result is " +
            std::to_string(bufferView.byteOffset + bufferView.byteLength) +
            ", which is more than the available " +
            std::to_string(buffer.cesium.data.size()) + " bytes.");
        continue;
      }

      const gsl::span<const std::byte> bufferSpan(buffer.cesium.data);
      const gsl::span<const std::byte> bufferViewSpan = bufferSpan.subspan(
          static_cast<size_t>(bufferView.byteOffset),
          static_cast<size_t>(bufferView.byteLength));
      ImageReaderResult imageResult = reader.readImage(bufferViewSpan);
      readModel.warnings.insert(
          readModel.warnings.end(),
          imageResult.warnings.begin(),
          imageResult.warnings.end());
      readModel.errors.insert(
          readModel.errors.end(),
          imageResult.errors.begin(),
          imageResult.errors.end());
      if (imageResult.image) {
        image.cesium = std::move(imageResult.image.value());
      } else {
        if (image.mimeType) {
          readModel.errors.emplace_back(
              "Declared image MIME Type: " + image.mimeType.value());
        } else {
          readModel.errors.emplace_back("Image does not declare a MIME Type");
        }
      }
    }
  }

  if (options.decodeDraco) {
    decodeDraco(readModel);
  }
}

} // namespace

GltfReader::GltfReader() : _context() {
  this->_context.registerExtension<
      MeshPrimitive,
      ExtensionKhrDracoMeshCompressionJsonHandler>();

  this->_context
      .registerExtension<Model, ExtensionModelExtFeatureMetadataJsonHandler>();
  this->_context.registerExtension<
      MeshPrimitive,
      ExtensionMeshPrimitiveExtFeatureMetadataJsonHandler>();
}

CesiumJsonReader::ExtensionReaderContext& GltfReader::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
GltfReader::getExtensions() const {
  return this->_context;
}

ModelReaderResult GltfReader::readModel(
    const gsl::span<const std::byte>& data,
    const ReadModelOptions& options) const {

  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  ModelReaderResult result = isBinaryGltf(data) ? readBinaryModel(context, data)
                                                : readJsonModel(context, data);

  if (result.model) {
    postprocess(*this, result, options);
  }

  return result;
}

ImageReaderResult
GltfReader::readImage(const gsl::span<const std::byte>& data) const {
  CESIUM_TRACE("CesiumGltf::readImage");

  ImageReaderResult result;

  result.image.emplace();
  ImageCesium& image = result.image.value();

  image.bytesPerChannel = 1;
  image.channels = 4;

  int channelsInFile;
  stbi_uc* pImage = stbi_load_from_memory(
      reinterpret_cast<const stbi_uc*>(data.data()),
      static_cast<int>(data.size()),
      &image.width,
      &image.height,
      &channelsInFile,
      image.channels);
  if (pImage) {
    CESIUM_TRACE(
        "copy image " + std::to_string(image.width) + "x" +
        std::to_string(image.height) + "x" + std::to_string(image.channels) +
        "x" + std::to_string(image.bytesPerChannel));
    // std::uint8_t is not implicitly convertible to std::byte, so we must use
    // reinterpret_cast to (safely) force the conversion.
    const auto lastByte =
        image.width * image.height * image.channels * image.bytesPerChannel;
    image.pixelData.resize(static_cast<std::size_t>(lastByte));
    std::uint8_t* u8Pointer =
        reinterpret_cast<std::uint8_t*>(image.pixelData.data());
    std::copy(pImage, pImage + lastByte, u8Pointer);
    stbi_image_free(pImage);
  } else {
    result.image.reset();
    result.errors.emplace_back(stbi_failure_reason());
  }

  return result;
}
