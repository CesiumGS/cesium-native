#include "ModelJsonHandler.h"
#include "applyKhrTextureTransform.h"
#include "decodeDataUrls.h"
#include "decodeDraco.h"
#include "decodeMeshOpt.h"
#include "dequantizeMeshData.h"
#include "registerReaderExtensions.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionTextureWebp.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumGltfReader/ImageDecoder.h>
#include <CesiumGltfReader/NetworkImageAssetDescriptor.h>
#include <CesiumGltfReader/NetworkSchemaAssetDescriptor.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
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

bool isBinaryGltf(const std::span<const std::byte>& data) noexcept {
  if (data.size() < sizeof(GlbHeader)) {
    return false;
  }

  return reinterpret_cast<const GlbHeader*>(data.data())->magic == 0x46546C67;
}

GltfReaderResult readJsonGltf(
    const CesiumJsonReader::JsonReaderOptions& context,
    const std::span<const std::byte>& data) {

  CESIUM_TRACE("CesiumGltfReader::GltfReader::readJsonGltf");

  ModelJsonHandler modelHandler(context);
  CesiumJsonReader::ReadJsonResult<Model> jsonResult =
      CesiumJsonReader::JsonReader::readJson(data, modelHandler);

  return GltfReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

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

GltfReaderResult readBinaryGltf(
    const CesiumJsonReader::JsonReaderOptions& context,
    const std::span<const std::byte>& data) {
  CESIUM_TRACE("CesiumGltfReader::GltfReader::readBinaryGltf");

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

  const std::span<const std::byte> glbData = data.subspan(0, pHeader->length);

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

  const std::span<const std::byte> jsonChunk =
      glbData.subspan(jsonStart, pJsonChunkHeader->chunkLength);
  std::span<const std::byte> binaryChunk;

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

  GltfReaderResult result = readJsonGltf(context, jsonChunk);

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
    if (buffer.byteLength > binaryChunkSize) {
      result.errors.emplace_back(
          "The size of the first buffer in the JSON chunk is " +
          std::to_string(buffer.byteLength) +
          ", which is larger than the size of the GLB binary chunk (" +
          std::to_string(binaryChunkSize) + ")");
      return result;
    }
    // The byte length of the BIN chunk MAY be up to 3 bytes
    // bigger than JSON-defined buffer.byteLength. When it is
    // more than 3 bytes bigger, generate a warning.
    if (binaryChunkSize - buffer.byteLength > 3) {
      result.warnings.emplace_back(
          "The size of the first buffer in the JSON chunk is " +
          std::to_string(buffer.byteLength) +
          ", which is more than 3 bytes smaller than the size of the GLB "
          "binary chunk (" +
          std::to_string(binaryChunkSize) + ")");
    }

    buffer.cesium.data = std::vector<std::byte>(
        binaryChunk.begin(),
        binaryChunk.begin() + buffer.byteLength);
  }

  return result;
}

void postprocess(GltfReaderResult& readGltf, const GltfReaderOptions& options) {
  if (!readGltf.model) {
    return;
  }

  Model& model = readGltf.model.value();

  auto extFeatureMetadataIter = std::find(
      model.extensionsUsed.begin(),
      model.extensionsUsed.end(),
      "EXT_feature_metadata");

  if (extFeatureMetadataIter != model.extensionsUsed.end()) {
    readGltf.warnings.emplace_back(
        "glTF contains EXT_feature_metadata extension, which is no longer "
        "supported. The model will still be loaded, but views cannot be "
        "constructed on its metadata.");
  }

  if (options.decodeDataUrls) {
    decodeDataUrls(readGltf, options);
  }

  if (options.decodeEmbeddedImages) {
    CESIUM_TRACE("CesiumGltfReader::decodeEmbeddedImages");
    for (Image& image : model.images) {
      // Ignore external images for now.
      if (image.uri) {
        continue;
      }

      // Image has already been decoded
      if (image.pAsset && !image.pAsset->pixelData.empty()) {
        continue;
      }

      const BufferView& bufferView =
          Model::getSafe(model.bufferViews, image.bufferView);
      const Buffer& buffer = Model::getSafe(model.buffers, bufferView.buffer);

      if (bufferView.byteOffset + bufferView.byteLength >
          static_cast<int64_t>(buffer.cesium.data.size())) {
        readGltf.warnings.emplace_back(
            "Image bufferView's byte offset is " +
            std::to_string(bufferView.byteOffset) + " and the byteLength is " +
            std::to_string(bufferView.byteLength) + ", the result is " +
            std::to_string(bufferView.byteOffset + bufferView.byteLength) +
            ", which is more than the available " +
            std::to_string(buffer.cesium.data.size()) + " bytes.");
        continue;
      }

      const std::span<const std::byte> bufferSpan(buffer.cesium.data);
      const std::span<const std::byte> bufferViewSpan = bufferSpan.subspan(
          static_cast<size_t>(bufferView.byteOffset),
          static_cast<size_t>(bufferView.byteLength));
      ImageReaderResult imageResult =
          ImageDecoder::readImage(bufferViewSpan, options.ktx2TranscodeTargets);
      readGltf.warnings.insert(
          readGltf.warnings.end(),
          imageResult.warnings.begin(),
          imageResult.warnings.end());
      readGltf.errors.insert(
          readGltf.errors.end(),
          imageResult.errors.begin(),
          imageResult.errors.end());
      if (imageResult.pImage) {
        image.pAsset = imageResult.pImage;
      } else {
        if (image.mimeType) {
          readGltf.errors.emplace_back(
              "Declared image MIME Type: " + image.mimeType.value());
        } else {
          readGltf.errors.emplace_back("Image does not declare a MIME Type");
        }
      }
    }

    // Copy the source property in texture extensions to the main Texture. The
    // image has already been decoded as necessary, so it's more convenient for
    // clients to not need to worry about the extension.
    for (Texture& texture : model.textures) {
      ExtensionTextureWebp* pWebP =
          texture.getExtension<ExtensionTextureWebp>();
      if (pWebP) {
        texture.source = pWebP->source;
      }

      ExtensionKhrTextureBasisu* pKtx =
          texture.getExtension<ExtensionKhrTextureBasisu>();
      if (pKtx) {
        texture.source = pKtx->source;
      }
    }
  }

  if (options.decodeDraco) {
    decodeDraco(readGltf);
  }

  if (options.decodeMeshOptData &&
      std::find(
          model.extensionsUsed.begin(),
          model.extensionsUsed.end(),
          "EXT_meshopt_compression") != model.extensionsUsed.end()) {
    decodeMeshOpt(model, readGltf);
  }

  if (options.dequantizeMeshData &&
      std::find(
          model.extensionsUsed.begin(),
          model.extensionsUsed.end(),
          "KHR_mesh_quantization") != model.extensionsUsed.end()) {
    dequantizeMeshData(model);
  }

  if (options.applyTextureTransform &&
      std::find(
          model.extensionsUsed.begin(),
          model.extensionsUsed.end(),
          "KHR_texture_transform") != model.extensionsUsed.end()) {
    applyKhrTextureTransform(model);
  }
}

} // namespace

GltfReader::GltfReader() : _context() {
  registerReaderExtensions(this->_context);
}

CesiumJsonReader::JsonReaderOptions& GltfReader::getOptions() {
  return this->_context;
}

const CesiumJsonReader::JsonReaderOptions& GltfReader::getExtensions() const {
  return this->_context;
}

GltfReaderResult GltfReader::readGltf(
    const std::span<const std::byte>& data,
    const GltfReaderOptions& options) const {

  const CesiumJsonReader::JsonReaderOptions& context = this->getExtensions();
  GltfReaderResult result = isBinaryGltf(data) ? readBinaryGltf(context, data)
                                               : readJsonGltf(context, data);

  if (result.model) {
    postprocess(result, options);
  }

  return result;
}

CesiumAsync::Future<GltfReaderResult> GltfReader::loadGltf(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& uri,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const GltfReaderOptions& options) const {
  return pAssetAccessor->get(asyncSystem, uri, headers)
      .thenInWorkerThread(
          [this, options, asyncSystem, pAssetAccessor, uri](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
            const CesiumAsync::IAssetResponse* pResponse = pRequest->response();

            if (!pResponse) {
              return asyncSystem.createResolvedFuture(GltfReaderResult{
                  std::nullopt,
                  {fmt::format("Request for {} failed.", uri)},
                  {}});
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              return asyncSystem.createResolvedFuture(GltfReaderResult{
                  std::nullopt,
                  {fmt::format(
                      "Request for {} failed with code {}",
                      uri,
                      pResponse->statusCode())},
                  {}});
            }

            const CesiumJsonReader::JsonReaderOptions& context =
                this->getExtensions();
            GltfReaderResult result =
                isBinaryGltf(pResponse->data())
                    ? readBinaryGltf(context, pResponse->data())
                    : readJsonGltf(context, pResponse->data());

            if (!result.model) {
              return asyncSystem.createResolvedFuture(std::move(result));
            }

            return resolveExternalData(
                asyncSystem,
                uri,
                pRequest->headers(),
                pAssetAccessor,
                options,
                std::move(result));
          })
      .thenInWorkerThread([options](GltfReaderResult&& result) {
        postprocess(result, options);
        return std::move(result);
      });
}

void CesiumGltfReader::GltfReader::postprocessGltf(
    GltfReaderResult& readGltf,
    const GltfReaderOptions& options) {
  if (readGltf.model) {
    postprocess(readGltf, options);
  }
}

/*static*/ Future<GltfReaderResult> GltfReader::resolveExternalData(
    const AsyncSystem& asyncSystem,
    const std::string& baseUrl,
    const HttpHeaders& headers,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const GltfReaderOptions& options,
    GltfReaderResult&& result) {

  // TODO: Can we avoid this copy conversion?
  std::vector<IAssetAccessor::THeader> tHeaders(headers.begin(), headers.end());

  if (!result.model) {
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  // Get a rough count of how many external buffers we may have.
  // Some of these may be data uris though.
  size_t uriBuffersCount = 0;
  for (const Buffer& buffer : result.model->buffers) {
    if (buffer.uri) {
      ++uriBuffersCount;
    }
  }

  for (const Image& image : result.model->images) {
    if (image.uri) {
      ++uriBuffersCount;
    }
  }

  {
    // We need to obtain the extension to find out if we have another buffer we
    // need to resolve. We can't use this pointer later since the result is
    // moved, so we'll do it twice.
    ExtensionModelExtStructuralMetadata* pStructuralMetadataTemp =
        result.model->getExtension<ExtensionModelExtStructuralMetadata>();

    if (pStructuralMetadataTemp &&
        pStructuralMetadataTemp->schemaUri.has_value()) {
      ++uriBuffersCount;
    }
  }

  if (uriBuffersCount == 0) {
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  auto pResult = std::make_unique<GltfReaderResult>(std::move(result));

  struct ExternalBufferLoadResult {
    bool success = false;
    std::string bufferUri;
    ErrorList warningsAndErrors;
  };

  std::vector<Future<ExternalBufferLoadResult>> resolvedBuffers;
  resolvedBuffers.reserve(uriBuffersCount);

  // We need to skip data uris.
  constexpr std::string_view dataPrefix = "data:";
  constexpr size_t dataPrefixLength = dataPrefix.size();

  // We already checked pResult->model at the top of the method, but clang-tidy
  // doesn't understand this.
  // NOLINTBEGIN(bugprone-unchecked-optional-access)

  for (Buffer& buffer : pResult->model->buffers) {
    if (buffer.uri && buffer.uri->substr(0, dataPrefixLength) != dataPrefix) {
      resolvedBuffers.push_back(
          pAssetAccessor
              ->get(asyncSystem, Uri::resolve(baseUrl, *buffer.uri), tHeaders)
              .thenInWorkerThread([pBuffer =
                                       &buffer](std::shared_ptr<IAssetRequest>&&
                                                    pRequest) {
                std::string bufferUri = pRequest->url();

                const IAssetResponse* pResponse = pRequest->response();
                if (!pResponse) {
                  return ExternalBufferLoadResult{
                      false,
                      bufferUri,
                      ErrorList::error("Request failed.")};
                }

                uint16_t statusCode = pResponse->statusCode();
                if (statusCode != 0 &&
                    (statusCode < 200 || statusCode >= 300)) {
                  return ExternalBufferLoadResult{
                      false,
                      bufferUri,
                      ErrorList::error(
                          fmt::format("Received status code {}.", statusCode))};
                }

                pBuffer->uri = std::nullopt;
                pBuffer->cesium.data = std::vector<std::byte>(
                    pResponse->data().begin(),
                    pResponse->data().end());
                return ExternalBufferLoadResult{true, bufferUri, ErrorList()};
              }));
    }
  }

  if (options.resolveExternalImages) {
    for (Image& image : pResult->model->images) {
      if (image.uri && image.uri->substr(0, dataPrefixLength) != dataPrefix) {
        const std::string uri = Uri::resolve(baseUrl, *image.uri);

        auto getAsset =
            [&options](
                const AsyncSystem& asyncSystem,
                const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
                const std::string& uri,
                const std::vector<IAssetAccessor::THeader>& headers)
            -> SharedFuture<ResultPointer<ImageAsset>> {
          NetworkImageAssetDescriptor assetKey{
              {uri, headers},
              options.ktx2TranscodeTargets};

          if (options.pSharedAssetSystem == nullptr ||
              options.pSharedAssetSystem->pImage == nullptr) {
            // We don't have a depot, so fetch this asset directly.
            return assetKey.load(asyncSystem, pAssetAccessor).share();
          } else {
            // We have a depot, so fetch this asset via that depot.
            return options.pSharedAssetSystem->pImage->getOrCreate(
                asyncSystem,
                pAssetAccessor,
                assetKey);
          }
        };

        SharedFuture<ResultPointer<ImageAsset>> future =
            getAsset(asyncSystem, pAssetAccessor, uri, tHeaders);

        resolvedBuffers.push_back(future.thenInWorkerThread(
            [pImage = &image,
             uri](const ResultPointer<ImageAsset>& loadedImage) {
              pImage->uri = std::nullopt;

              if (loadedImage.pValue) {
                pImage->pAsset = loadedImage.pValue;
                return ExternalBufferLoadResult{true, uri, loadedImage.errors};
              }

              return ExternalBufferLoadResult{false, uri, loadedImage.errors};
            }));
      }
    }
  }

  ExtensionModelExtStructuralMetadata* pStructuralMetadata =
      pResult->model->getExtension<ExtensionModelExtStructuralMetadata>();
  // NOLINTEND(bugprone-unchecked-optional-access)

  if (options.resolveExternalStructuralMetadata && pStructuralMetadata &&
      pStructuralMetadata->schemaUri.has_value()) {

    auto getAsset = [&options](
                        const AsyncSystem& asyncSystem,
                        const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
                        const std::string& uri,
                        const std::vector<IAssetAccessor::THeader>& headers)
        -> SharedFuture<ResultPointer<Schema>> {
      NetworkSchemaAssetDescriptor assetKey{{uri, headers}};

      if (options.pSharedAssetSystem == nullptr ||
          options.pSharedAssetSystem->pExternalMetadataSchema == nullptr) {
        // We don't have a depot, so fetch this asset directly.
        return assetKey.load(asyncSystem, pAssetAccessor).share();
      } else {
        // We have a depot, so fetch this asset via that depot.
        return options.pSharedAssetSystem->pExternalMetadataSchema->getOrCreate(
            asyncSystem,
            pAssetAccessor,
            assetKey);
      }
    };

    std::string uri = Uri::resolve(baseUrl, *pStructuralMetadata->schemaUri);

    SharedFuture<ResultPointer<Schema>> future =
        getAsset(asyncSystem, pAssetAccessor, uri, tHeaders);

    resolvedBuffers.push_back(future.thenInWorkerThread(
        [pStructuralMetadata = pStructuralMetadata,
         uri](const ResultPointer<CesiumGltf::Schema>& loadedSchema) {
          pStructuralMetadata->schemaUri = std::nullopt;

          if (loadedSchema.pValue) {
            pStructuralMetadata->schema = loadedSchema.pValue;
            return ExternalBufferLoadResult{true, uri, loadedSchema.errors};
          }

          return ExternalBufferLoadResult{false, uri, loadedSchema.errors};
        }));
  }

  return asyncSystem.all(std::move(resolvedBuffers))
      .thenInWorkerThread(
          [pResult = std::move(pResult)](
              std::vector<ExternalBufferLoadResult>&& loadResults) mutable {
            for (auto& bufferResult : loadResults) {
              if (!bufferResult.success) {
                pResult->warnings.push_back(
                    "Could not load the external glTF buffer: " +
                    bufferResult.bufferUri);
              }

              if (!bufferResult.warningsAndErrors.errors.empty()) {
                pResult->warnings.emplace_back(fmt::format(
                    "Errors while loading external glTF buffer: {}\n- {}",
                    bufferResult.bufferUri,
                    CesiumUtility::joinToString(
                        bufferResult.warningsAndErrors.errors,
                        "\n- ")));
              }

              if (!bufferResult.warningsAndErrors.warnings.empty()) {
                pResult->warnings.emplace_back(fmt::format(
                    "Warnings while loading external glTF buffer: {}\n- {}",
                    bufferResult.bufferUri,
                    CesiumUtility::joinToString(
                        bufferResult.warningsAndErrors.warnings,
                        "\n- ")));
              }
            }

            return std::move(*pResult.release());
          });
}

/*static*/ ImageReaderResult GltfReader::readImage(
    const std::span<const std::byte>& data,
    const Ktx2TranscodeTargets& ktx2TranscodeTargets) {
  return ImageDecoder::readImage(data, ktx2TranscodeTargets);
}

/*static*/ std::optional<std::string>
GltfReader::generateMipMaps(CesiumGltf::ImageAsset& image) {
  return ImageDecoder::generateMipMaps(image);
}
