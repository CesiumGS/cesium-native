#include "CesiumGltf/GltfReader.h"
#include "CesiumGltf/IExtensionJsonHandler.h"
#include "CesiumGltf/ReaderContext.h"
#include "CesiumJsonReader/JsonHandler.h"
#include "CesiumJsonReader/JsonReader.h"
#include "CesiumUtility/Tracing.h"
#include "KHR_draco_mesh_compressionJsonHandler.h"
#include "ModelJsonHandler.h"
#include "decodeDataUrls.h"
#include "decodeDraco.h"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <rapidjson/reader.h>
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

bool isBinaryGltf(const gsl::span<const std::byte>& data) {
  if (data.size() < sizeof(GlbHeader)) {
    return false;
  }

  return reinterpret_cast<const GlbHeader*>(data.data())->magic == 0x46546C67;
}

ModelReaderResult readJsonModel(
    const ReaderContext& context,
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
  unsigned char c0 = static_cast<unsigned char>(i & 0xFF);
  unsigned char c1 = static_cast<unsigned char>((i >> 8) & 0xFF);
  unsigned char c2 = static_cast<unsigned char>((i >> 16) & 0xFF);
  unsigned char c3 = static_cast<unsigned char>((i >> 24) & 0xFF);
  std::stringstream stream;
  stream << c0 << c1 << c2 << c3 << " (0x" << std::hex << i << ")";
  return stream.str();
}
} // namespace

ModelReaderResult readBinaryModel(
    const ReaderContext& context,
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

  gsl::span<const std::byte> glbData = data.subspan(0, pHeader->length);

  const ChunkHeader* pJsonChunkHeader =
      reinterpret_cast<const ChunkHeader*>(glbData.data() + sizeof(GlbHeader));
  if (pJsonChunkHeader->chunkType != 0x4E4F534A) {
    return {
        std::nullopt,
        {"GLB JSON chunk does not have the expected chunkType 'JSON', but " +
         toMagicString(pJsonChunkHeader->chunkType)},
        {}};
  }

  size_t jsonStart = sizeof(GlbHeader) + sizeof(ChunkHeader);
  size_t jsonEnd = jsonStart + pJsonChunkHeader->chunkLength;

  if (jsonEnd > glbData.size()) {
    return {
        std::nullopt,
        {"GLB JSON chunk extends past the end of the buffer, JSON end at " +
         std::to_string(jsonEnd) + ", data size " +
         std::to_string(glbData.size())},
        {}};
  }

  gsl::span<const std::byte> jsonChunk =
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

    size_t binaryStart = jsonEnd + sizeof(ChunkHeader);
    size_t binaryEnd = binaryStart + pBinaryChunkHeader->chunkLength;

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

    int64_t binaryChunkSize = static_cast<int64_t>(binaryChunk.size());
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
    ReaderContext& context,
    ModelReaderResult& readModel,
    const ReadModelOptions& options) {
  Model& model = readModel.model.value();

  if (options.decodeDataUrls) {
    decodeDataUrls(context, readModel, options.clearDecodedDataUrls);
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

      gsl::span<const std::byte> bufferSpan(buffer.cesium.data);
      gsl::span<const std::byte> bufferViewSpan = bufferSpan.subspan(
          static_cast<size_t>(bufferView.byteOffset),
          static_cast<size_t>(bufferView.byteLength));
      ImageReaderResult imageResult = context.reader.readImage(bufferViewSpan);
      if (imageResult.image) {
        image.cesium = std::move(imageResult.image.value());
      }
    }
  }

  if (options.decodeDraco) {
    decodeDraco(readModel);
  }
}

class AnyExtensionJsonHandler : public JsonObjectJsonHandler,
                                public IExtensionJsonHandler {
public:
  AnyExtensionJsonHandler(const ReaderContext& /* context */)
      : JsonObjectJsonHandler() {}

  virtual void reset(
      IJsonHandler* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) override {
    std::any& value =
        o.extensions.emplace(extensionName, JsonValue(JsonValue::Object()))
            .first->second;
    JsonObjectJsonHandler::reset(
        pParentHandler,
        std::any_cast<JsonValue>(&value));
  }

  virtual IJsonHandler* readNull() override {
    return JsonObjectJsonHandler::readNull();
  };
  virtual IJsonHandler* readBool(bool b) override {
    return JsonObjectJsonHandler::readBool(b);
  }
  virtual IJsonHandler* readInt32(int32_t i) override {
    return JsonObjectJsonHandler::readInt32(i);
  }
  virtual IJsonHandler* readUint32(uint32_t i) override {
    return JsonObjectJsonHandler::readUint32(i);
  }
  virtual IJsonHandler* readInt64(int64_t i) override {
    return JsonObjectJsonHandler::readInt64(i);
  }
  virtual IJsonHandler* readUint64(uint64_t i) override {
    return JsonObjectJsonHandler::readUint64(i);
  }
  virtual IJsonHandler* readDouble(double d) override {
    return JsonObjectJsonHandler::readDouble(d);
  }
  virtual IJsonHandler* readString(const std::string_view& str) override {
    return JsonObjectJsonHandler::readString(str);
  }
  virtual IJsonHandler* readObjectStart() override {
    return JsonObjectJsonHandler::readObjectStart();
  }
  virtual IJsonHandler* readObjectKey(const std::string_view& str) override {
    return JsonObjectJsonHandler::readObjectKey(str);
  }
  virtual IJsonHandler* readObjectEnd() override {
    return JsonObjectJsonHandler::readObjectEnd();
  }
  virtual IJsonHandler* readArrayStart() override {
    return JsonObjectJsonHandler::readArrayStart();
  }
  virtual IJsonHandler* readArrayEnd() override {
    return JsonObjectJsonHandler::readArrayEnd();
  }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context =
          std::vector<std::string>()) override {
    JsonObjectJsonHandler::reportWarning(warning, std::move(context));
  }
};

} // namespace

GltfReader::GltfReader() {
  this->registerExtension<
      MeshPrimitive,
      KHR_draco_mesh_compressionJsonHandler>();
}

void GltfReader::setExtensionState(
    const std::string& extensionName,
    ExtensionState newState) {
  this->_extensionStates[extensionName] = newState;
}

ModelReaderResult GltfReader::readModel(
    const gsl::span<const std::byte>& data,
    const ReadModelOptions& options) const {

  ReaderContext context{*this};
  ModelReaderResult result = isBinaryGltf(data) ? readBinaryModel(context, data)
                                                : readJsonModel(context, data);

  if (result.model) {
    postprocess(context, result, options);
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

std::unique_ptr<IExtensionJsonHandler> GltfReader::createExtensionHandler(
    const ReaderContext& context,
    const std::string_view& extensionName,
    const std::string& extendedObjectType) const {

  std::string extensionNameString{extensionName};

  auto stateIt = this->_extensionStates.find(extensionNameString);
  if (stateIt != this->_extensionStates.end()) {
    if (stateIt->second == ExtensionState::Disabled) {
      return nullptr;
    } else if (stateIt->second == ExtensionState::JsonOnly) {
      return std::make_unique<AnyExtensionJsonHandler>(context);
    }
  }

  auto extensionNameIt = this->_extensions.find(extensionNameString);
  if (extensionNameIt == this->_extensions.end()) {
    return std::make_unique<AnyExtensionJsonHandler>(context);
  }

  auto objectTypeIt = extensionNameIt->second.find(extendedObjectType);
  if (objectTypeIt == extensionNameIt->second.end()) {
    return std::make_unique<AnyExtensionJsonHandler>(context);
  }

  return objectTypeIt->second(context);
}
