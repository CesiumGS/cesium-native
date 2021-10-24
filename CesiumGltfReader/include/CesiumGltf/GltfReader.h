#pragma once

#include "ReaderLibrary.h"

#include <CesiumGltf/Model.h>
#include <CesiumJsonReader/ExtensionReaderContext.h>
#include <CesiumJsonReader/IExtensionJsonHandler.h>

#include <gsl/span>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGltf {

/**
 * @brief The result of reading a glTF model with
 * {@link GltfReader::readModel}.
 */
struct CESIUMGLTFREADER_API ModelReaderResult {
  /**
   * @brief The read model, or std::nullopt if the model could not be read.
   */
  std::optional<Model> model;

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
  std::optional<ImageCesium> image;

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
struct CESIUMGLTFREADER_API ReadModelOptions {
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
  ModelReaderResult readModel(
      const gsl::span<const std::byte>& data,
      const ReadModelOptions& options = ReadModelOptions()) const;

  /**
   * @brief Reads an image from a buffer.
   *
   * The [stb_image](https://github.com/nothings/stb) library is used to decode
   * images in `JPG`, `PNG`, `TGA`, `BMP`, `PSD`, `GIF`, `HDR`, or `PIC` format.
   *
   * @param data The buffer from which to read the image.
   * @return The result of reading the image.
   */
  ImageReaderResult readImage(const gsl::span<const std::byte>& data) const;

private:
  CesiumJsonReader::ExtensionReaderContext _context;
};

} // namespace CesiumGltf
