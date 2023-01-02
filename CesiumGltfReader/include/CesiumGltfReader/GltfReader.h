#pragma once

#include "CesiumGltfReader/Library.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltf/Model.h>
#include <CesiumJsonReader/ExtensionReaderContext.h>
#include <CesiumJsonReader/IExtensionJsonHandler.h>

#include <gsl/span>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGltfReader {

/**
 * @brief The result of reading a glTF model with
 * {@link GltfReader::readGltf}.
 */
struct CESIUMGLTFREADER_API GltfReaderResult {
  /**
   * @brief The read model, or std::nullopt if the model could not be read.
   */
  std::optional<CesiumGltf::Model> model;

  /**
   * @brief Errors, if any, that occurred during the load process.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warnings, if any, that occurred during the load process.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief The result of reading an image with
 * {@link GltfReader::readImage}.
 */
struct CESIUMGLTFREADER_API ImageReaderResult {

  /**
   * @brief The {@link ImageCesium} that was read.
   *
   * This will be `std::nullopt` if the image could not be read.
   */
  std::optional<CesiumGltf::ImageCesium> image;

  /**
   * @brief Error messages that occurred while trying to read the image.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warning messages that occurred while reading the image.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief Options for how to read a glTF.
 */
struct CESIUMGLTFREADER_API GltfReaderOptions {
  /**
   * @brief Whether data URLs in buffers and images should be automatically
   * decoded as part of the load process.
   */
  bool decodeDataUrls = true;

  /**
   * @brief Whether data URLs should be cleared after they are successfully
   * decoded.
   *
   * This reduces the memory usage of the model.
   */
  bool clearDecodedDataUrls = true;

  /**
   * @brief Whether embedded images in {@link Model::buffers} should be
   * automatically decoded as part of the load process.
   *
   * The {@link ImageSpec::mimeType} property is ignored, and instead the
   * [stb_image](https://github.com/nothings/stb) library is used to decode
   * images in `JPG`, `PNG`, `TGA`, `BMP`, `PSD`, `GIF`, `HDR`, or `PIC` format.
   */
  bool decodeEmbeddedImages = true;

  /**
   * @brief Whether geometry compressed using the `KHR_draco_mesh_compression`
   * extension should be automatically decoded as part of the load process.
   */
  bool decodeDraco = true;

  /**
   * @brief For each possible input transmission format, this struct names
   * the ideal target gpu-compressed pixel format to transcode to.
   */
  CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets;
};

/**
 * @brief Reads glTF models and images.
 */
class CESIUMGLTFREADER_API GltfReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  GltfReader();

  /**
   * @brief Gets the context used to control how extensions are loaded from glTF
   * files.
   */
  CesiumJsonReader::ExtensionReaderContext& getExtensions();

  /**
   * @brief Gets the context used to control how extensions are loaded from glTF
   * files.
   */
  const CesiumJsonReader::ExtensionReaderContext& getExtensions() const;

  /**
   * @brief Reads a glTF or binary glTF (GLB) from a buffer.
   *
   * @param data The buffer from which to read the glTF.
   * @param options Options for how to read the glTF.
   * @return The result of reading the glTF.
   */
  GltfReaderResult readGltf(
      const gsl::span<const std::byte>& data,
      const GltfReaderOptions& options = GltfReaderOptions()) const;

  /**
   * @brief Accepts the result of {@link readGltf} and resolves any remaining
   * external buffers and images.
   *
   * @param asyncSystem The async system to use for resolving external data.
   * @param baseUrl The base url that all the external uris are relative to.
   * @param headers The http headers needed to make any external data requests.
   * @param pAssetAccessor The asset accessor to use to request the external
   * buffers and images.
   * @param options Options for how to read the glTF.
   * @param result The result of the synchronous readGltf invocation.
   */
  static CesiumAsync::Future<GltfReaderResult> resolveExternalData(
      CesiumAsync::AsyncSystem asyncSystem,
      const std::string& baseUrl,
      const CesiumAsync::HttpHeaders& headers,
      std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor,
      const GltfReaderOptions& options,
      GltfReaderResult&& result);

  /**
   * @brief Reads an image from a buffer.
   *
   * The [stb_image](https://github.com/nothings/stb) library is used to decode
   * images in `JPG`, `PNG`, `TGA`, `BMP`, `PSD`, `GIF`, `HDR`, or `PIC` format.
   *
   * @param data The buffer from which to read the image.
   * @param ktx2TranscodeTargetFormat The compression format to transcode
   * KTX v2 textures into. If this is std::nullopt, KTX v2 textures will be
   * fully decompressed into raw pixels.
   * @return The result of reading the image.
   */
  static ImageReaderResult readImage(
      const gsl::span<const std::byte>& data,
      const CesiumGltf::Ktx2TranscodeTargets& ktx2TranscodeTargets);

  /**
   * @brief Generate mipmaps for this image.
   *
   * Does nothing if mipmaps already exist or the compressedPixelFormat is not
   * GpuCompressedPixelFormat::NONE.
   *
   * @param image The image to generate mipmaps for.   *
   * @return A string describing the error, if unable to generate mipmaps.
   */
  static std::optional<std::string>
  generateMipMaps(CesiumGltf::ImageCesium& image);

private:
  CesiumJsonReader::ExtensionReaderContext _context;
};

} // namespace CesiumGltfReader
