#include "CesiumGltf/Reader.h"
#include "CesiumGltf/IExtensionJsonReader.h"
#include "CesiumGltf/ReaderContext.h"
#include "CesiumJsonReader/JsonReader.h"
#include "KHR_draco_mesh_compressionJsonHandler.h"
#include "ModelJsonHandler.h"
#include "decodeDataUrls.h"
#include "decodeDraco.h"
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
struct Dispatcher {
  IJsonReader* pCurrent;

  bool update(IJsonReader* pNext) {
    if (pNext == nullptr) {
      return false;
    }

    this->pCurrent = pNext;
    return true;
  }

  bool Null() { return update(pCurrent->readNull()); }
  bool Bool(bool b) { return update(pCurrent->readBool(b)); }
  bool Int(int i) { return update(pCurrent->readInt32(i)); }
  bool Uint(unsigned i) { return update(pCurrent->readUint32(i)); }
  bool Int64(int64_t i) { return update(pCurrent->readInt64(i)); }
  bool Uint64(uint64_t i) { return update(pCurrent->readUint64(i)); }
  bool Double(double d) { return update(pCurrent->readDouble(d)); }
  bool RawNumber(const char* /* str */, size_t /* length */, bool /* copy */) {
    // This should not be called.
    assert(false);
    return false;
  }
  bool String(const char* str, size_t length, bool /* copy */) {
    return update(pCurrent->readString(std::string_view(str, length)));
  }
  bool StartObject() { return update(pCurrent->readObjectStart()); }
  bool Key(const char* str, size_t length, bool /* copy */) {
    return update(pCurrent->readObjectKey(std::string_view(str, length)));
  }
  bool EndObject(size_t /* memberCount */) {
    return update(pCurrent->readObjectEnd());
  }
  bool StartArray() { return update(pCurrent->readArrayStart()); }
  bool EndArray(size_t /* elementCount */) {
    return update(pCurrent->readArrayEnd());
  }
};

class FinalJsonHandler : public ObjectJsonHandler {
public:
  FinalJsonHandler(
      const ReaderContext& /* context */,
      ModelReaderResult& result,
      rapidjson::MemoryStream& inputStream)
      : ObjectJsonHandler(), _result(result), _inputStream(inputStream) {
    reset(this);
  }

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context) override {
    std::string fullWarning = warning;
    fullWarning += "\n  While parsing: ";
    for (auto it = context.rbegin(); it != context.rend(); ++it) {
      fullWarning += *it;
    }

    fullWarning += "\n  From byte offset: ";
    fullWarning += std::to_string(this->_inputStream.Tell());

    this->_result.warnings.emplace_back(std::move(fullWarning));
  }

private:
  ModelReaderResult& _result;
  rapidjson::MemoryStream& _inputStream;
};

std::string getMessageFromRapidJsonError(rapidjson::ParseErrorCode code) {
  switch (code) {
  case rapidjson::ParseErrorCode::kParseErrorDocumentEmpty:
    return "The document is empty.";
  case rapidjson::ParseErrorCode::kParseErrorDocumentRootNotSingular:
    return "The document root must not be followed by other values.";
  case rapidjson::ParseErrorCode::kParseErrorValueInvalid:
    return "Invalid value.";
  case rapidjson::ParseErrorCode::kParseErrorObjectMissName:
    return "Missing a name for object member.";
  case rapidjson::ParseErrorCode::kParseErrorObjectMissColon:
    return "Missing a colon after a name of object member.";
  case rapidjson::ParseErrorCode::kParseErrorObjectMissCommaOrCurlyBracket:
    return "Missing a comma or '}' after an object member.";
  case rapidjson::ParseErrorCode::kParseErrorArrayMissCommaOrSquareBracket:
    return "Missing a comma or ']' after an array element.";
  case rapidjson::ParseErrorCode::kParseErrorStringUnicodeEscapeInvalidHex:
    return "Incorrect hex digit after \\u escape in string.";
  case rapidjson::ParseErrorCode::kParseErrorStringUnicodeSurrogateInvalid:
    return "The surrogate pair in string is invalid.";
  case rapidjson::ParseErrorCode::kParseErrorStringEscapeInvalid:
    return "Invalid escape character in string.";
  case rapidjson::ParseErrorCode::kParseErrorStringMissQuotationMark:
    return "Missing a closing quotation mark in string.";
  case rapidjson::ParseErrorCode::kParseErrorStringInvalidEncoding:
    return "Invalid encoding in string.";
  case rapidjson::ParseErrorCode::kParseErrorNumberTooBig:
    return "Number too big to be stored in double.";
  case rapidjson::ParseErrorCode::kParseErrorNumberMissFraction:
    return "Missing fraction part in number.";
  case rapidjson::ParseErrorCode::kParseErrorNumberMissExponent:
    return "Missing exponent in number.";
  case rapidjson::ParseErrorCode::kParseErrorTermination:
    return "Parsing was terminated.";
  case rapidjson::ParseErrorCode::kParseErrorUnspecificSyntaxError:
  default:
    return "Unspecific syntax error.";
  }
}

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
  rapidjson::Reader reader;
  rapidjson::MemoryStream inputStream(
      reinterpret_cast<const char*>(data.data()),
      data.size());

  ModelReaderResult result;
  ModelJsonHandler modelHandler(context);
  FinalJsonHandler finalHandler(context, result, inputStream);
  Dispatcher dispatcher{&modelHandler};

  result.model.emplace();
  modelHandler.reset(&finalHandler, &result.model.value());

  reader.IterativeParseInit();

  bool success = true;
  while (success && !reader.IterativeParseComplete()) {
    success = reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(
        inputStream,
        dispatcher);
  }

  if (reader.HasParseError()) {
    result.model.reset();

    std::string s("glTF JSON parsing error at byte offset ");
    s += std::to_string(reader.GetErrorOffset());
    s += ": ";
    s += getMessageFromRapidJsonError(reader.GetParseErrorCode());
    result.errors.emplace_back(std::move(s));
  }

  return result;
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

    if (model.buffers.size() == 0) {
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

class AnyExtensionJsonReader : public JsonObjectJsonHandler,
                               public IExtensionJsonReader {
public:
  AnyExtensionJsonReader(const ReaderContext& /* context */)
      : JsonObjectJsonHandler() {}

  virtual void reset(
      IJsonReader* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) override {
    std::any& value =
        o.extensions.emplace(extensionName, JsonValue(JsonValue::Object()))
            .first->second;
    JsonObjectJsonHandler::reset(
        pParentHandler,
        &std::any_cast<JsonValue&>(value));
  }

  virtual IJsonReader* readNull() override {
    return JsonObjectJsonHandler::readNull();
  };
  virtual IJsonReader* readBool(bool b) override {
    return JsonObjectJsonHandler::readBool(b);
  }
  virtual IJsonReader* readInt32(int32_t i) override {
    return JsonObjectJsonHandler::readInt32(i);
  }
  virtual IJsonReader* readUint32(uint32_t i) override {
    return JsonObjectJsonHandler::readUint32(i);
  }
  virtual IJsonReader* readInt64(int64_t i) override {
    return JsonObjectJsonHandler::readInt64(i);
  }
  virtual IJsonReader* readUint64(uint64_t i) override {
    return JsonObjectJsonHandler::readUint64(i);
  }
  virtual IJsonReader* readDouble(double d) override {
    return JsonObjectJsonHandler::readDouble(d);
  }
  virtual IJsonReader* readString(const std::string_view& str) override {
    return JsonObjectJsonHandler::readString(str);
  }
  virtual IJsonReader* readObjectStart() override {
    return JsonObjectJsonHandler::readObjectStart();
  }
  virtual IJsonReader* readObjectKey(const std::string_view& str) override {
    return JsonObjectJsonHandler::readObjectKey(str);
  }
  virtual IJsonReader* readObjectEnd() override {
    return JsonObjectJsonHandler::readObjectEnd();
  }
  virtual IJsonReader* readArrayStart() override {
    return JsonObjectJsonHandler::readArrayStart();
  }
  virtual IJsonReader* readArrayEnd() override {
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

Reader::Reader() {
  this->registerExtension<
      MeshPrimitive,
      KHR_draco_mesh_compressionJsonHandler>();
}

void Reader::setExtensionState(
    const std::string& extensionName,
    ExtensionState newState) {
  this->_extensionStates[extensionName] = newState;
}

ModelReaderResult Reader::readModel(
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
Reader::readImage(const gsl::span<const std::byte>& data) const {
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
    image.pixelData.assign(
        pImage,
        pImage + image.width * image.height * image.channels *
                     image.bytesPerChannel);
    stbi_image_free(pImage);
  } else {
    result.image.reset();
    result.errors.emplace_back(stbi_failure_reason());
  }

  return result;
}

std::unique_ptr<IExtensionJsonReader> Reader::createExtensionReader(
    const ReaderContext& context,
    const std::string_view& extensionName,
    const std::string& extendedObjectType) const {

  std::string extensionNameString{extensionName};

  auto stateIt = this->_extensionStates.find(extensionNameString);
  if (stateIt != this->_extensionStates.end()) {
    if (stateIt->second == ExtensionState::Disabled) {
      return nullptr;
    } else if (stateIt->second == ExtensionState::JsonOnly) {
      return std::make_unique<AnyExtensionJsonReader>(context);
    }
  }

  auto extensionNameIt = this->_extensions.find(extensionNameString);
  if (extensionNameIt == this->_extensions.end()) {
    return std::make_unique<AnyExtensionJsonReader>(context);
  }

  auto objectTypeIt = extensionNameIt->second.find(extendedObjectType);
  if (objectTypeIt == extensionNameIt->second.end()) {
    return std::make_unique<AnyExtensionJsonReader>(context);
  }

  return objectTypeIt->second(context);
}
