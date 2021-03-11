#include "CesiumGltf/Reader.h"
#include "JsonHandler.h"
#include "ModelJsonHandler.h"
#include "decodeDataUrls.h"
#include "decodeDraco.h"
#include <algorithm>
#include <cstddef>
#include <rapidjson/reader.h>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>

using namespace CesiumGltf;

namespace {
struct Dispatcher {
  IJsonHandler* pCurrent;

  bool update(IJsonHandler* pNext) {
    if (pNext == nullptr) {
      return false;
    }

    this->pCurrent = pNext;
    return true;
  }

  bool Null() { return update(pCurrent->Null()); }
  bool Bool(bool b) { return update(pCurrent->Bool(b)); }
  bool Int(int i) { return update(pCurrent->Int(i)); }
  bool Uint(unsigned i) { return update(pCurrent->Uint(i)); }
  bool Int64(int64_t i) { return update(pCurrent->Int64(i)); }
  bool Uint64(uint64_t i) { return update(pCurrent->Uint64(i)); }
  bool Double(double d) { return update(pCurrent->Double(d)); }
  bool RawNumber(const char* str, size_t length, bool copy) {
    return update(pCurrent->RawNumber(str, length, copy));
  }
  bool String(const char* str, size_t length, bool copy) {
    return update(pCurrent->String(str, length, copy));
  }
  bool StartObject() { return update(pCurrent->StartObject()); }
  bool Key(const char* str, size_t length, bool copy) {
    return update(pCurrent->Key(str, length, copy));
  }
  bool EndObject(size_t memberCount) {
    return update(pCurrent->EndObject(memberCount));
  }
  bool StartArray() { return update(pCurrent->StartArray()); }
  bool EndArray(size_t elementCount) {
    return update(pCurrent->EndArray(elementCount));
  }
};

class FinalJsonHandler : public ObjectJsonHandler {
public:
  FinalJsonHandler(
      const ReadModelOptions& options,
      ModelReaderResult& result,
      rapidjson::MemoryStream& inputStream)
      : ObjectJsonHandler(options), _result(result), _inputStream(inputStream) {
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
    const gsl::span<const std::byte>& data,
    const ReadModelOptions& options) {
  rapidjson::Reader reader;
  rapidjson::MemoryStream inputStream(
      reinterpret_cast<const char*>(data.data()),
      data.size());

  ModelReaderResult result;
  ModelJsonHandler modelHandler(options);
  FinalJsonHandler finalHandler(options, result, inputStream);
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

ModelReaderResult readBinaryModel(
    const gsl::span<const std::byte>& data,
    const ReadModelOptions& options) {
  if (data.size() < sizeof(GlbHeader) + sizeof(ChunkHeader)) {
    return {std::nullopt, {"Too short to be a valid GLB."}, {}};
  }

  const GlbHeader* pHeader = reinterpret_cast<const GlbHeader*>(data.data());
  if (pHeader->magic != 0x46546C67) {
    return {
        std::nullopt,
        {"GLB does not start with the expected magic value 'glTF'."},
        {}};
  }

  if (pHeader->version != 2) {
    return {std::nullopt, {"Only binary glTF version 2 is supported."}, {}};
  }

  if (pHeader->length > data.size()) {
    return {std::nullopt, {"GLB extends past the end of the buffer."}, {}};
  }

  gsl::span<const std::byte> glbData = data.subspan(0, pHeader->length);

  const ChunkHeader* pJsonChunkHeader =
      reinterpret_cast<const ChunkHeader*>(glbData.data() + sizeof(GlbHeader));
  if (pJsonChunkHeader->chunkType != 0x4E4F534A) {
    return {
        std::nullopt,
        {"GLB JSON chunk does not have the expected chunkType."},
        {}};
  }

  size_t jsonStart = sizeof(GlbHeader) + sizeof(ChunkHeader);
  size_t jsonEnd = jsonStart + pJsonChunkHeader->chunkLength;

  if (jsonEnd > glbData.size()) {
    return {
        std::nullopt,
        {"GLB JSON chunk extends past the end of the buffer."},
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
          {"GLB binary chunk does not have the expected chunkType."},
          {}};
    }

    size_t binaryStart = jsonEnd + sizeof(ChunkHeader);
    size_t binaryEnd = binaryStart + pBinaryChunkHeader->chunkLength;

    if (binaryEnd > glbData.size()) {
      return {
          std::nullopt,
          {"GLB binary chunk extends past the end of the buffer."},
          {}};
    }

    binaryChunk = glbData.subspan(binaryStart, pBinaryChunkHeader->chunkLength);
  }

  ModelReaderResult result = readJsonModel(jsonChunk, options);

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
    ModelReaderResult& readModel,
    const ReadModelOptions& options) {
  Model& model = readModel.model.value();

  if (options.decodeDataUrls) {
    decodeDataUrls(readModel, options.clearDecodedDataUrls);
  }

  if (options.decodeEmbeddedImages) {
    for (Image& image : model.images) {
      const BufferView& bufferView =
          Model::getSafe(model.bufferViews, image.bufferView);
      const Buffer& buffer = Model::getSafe(model.buffers, bufferView.buffer);

      if (bufferView.byteOffset + bufferView.byteLength >
          static_cast<int64_t>(buffer.cesium.data.size())) {
        readModel.warnings.emplace_back(
            "Image bufferView's byteLength is more than the available bytes.");
        continue;
      }

      gsl::span<const std::byte> bufferSpan(buffer.cesium.data);
      gsl::span<const std::byte> bufferViewSpan = bufferSpan.subspan(
          static_cast<size_t>(bufferView.byteOffset),
          static_cast<size_t>(bufferView.byteLength));
      ImageReaderResult imageResult = readImage(bufferViewSpan);
      if (imageResult.image) {
        image.cesium = std::move(imageResult.image.value());
      }
    }
  }

  if (options.decodeDraco) {
    decodeDraco(readModel);
  }
}
} // namespace

ModelReaderResult CesiumGltf::readModel(
    const gsl::span<const std::byte>& data,
    const ReadModelOptions& options) {
  ModelReaderResult result = isBinaryGltf(data) ? readBinaryModel(data, options)
                                                : readJsonModel(data, options);

  if (result.model) {
    postprocess(result, options);
  }

  return result;
}

ImageReaderResult
CesiumGltf::readImage(const gsl::span<const std::byte>& data) {
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
    const int lastByte =
        image.width * image.height * image.channels * image.bytesPerChannel;
    std::transform(
        pImage,
        pImage + lastByte,
        std::back_inserter(image.pixelData),
        [](char c) { return std::byte(c); });
    stbi_image_free(pImage);
  } else {
    result.image.reset();
    result.errors.emplace_back(stbi_failure_reason());
  }

  return result;
}