#pragma once

#include "WriterLibrary.h"

#include <CesiumGltf/Model.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumJsonWriter/IExtensionJsonHandler.h>

#include <gsl/span>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CesiumGltf {

/**
 * @brief The result of writing a glTF model with
 * {@link GltfWriter::writeModel}.
 */
struct CESIUMGLTFWRITER_API ModelWriterResult {
  /**
   * @brief The write model, or std::nullopt if the model could not be write.
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
 * @brief The result of writing an image with
 * {@link GltfWriter::writeImage}.
 */
struct CESIUMGLTFWRITER_API ImageWriterResult {

  /**
   * @brief The {@link ImageCesium} that was write.
   *
   * This will be `std::nullopt` if the image could not be write.
   */
  std::optional<ImageCesium> image;

  /**
   * @brief Error messages that occurred while trying to write the image.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warning messages that occurred while writing the image.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief Options for how to write a glTF.
 */
struct CESIUMGLTFWRITER_API WriteModelOptions {
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
 * @brief Writes glTF models and images.
 */
class CESIUMGLTFWRITER_API GltfWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  GltfWriter();

  /**
   * @brief Gets the context used to control how extensions are loaded from glTF
   * files.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how extensions are loaded from glTF
   * files.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Writes a glTF or binary glTF (GLB) from a buffer.
   *
   * @param data The buffer from which to write the glTF.
   * @param options Options for how to write the glTF.
   * @return The result of writing the glTF.
   */
  ModelWriterResult writeModel(
      const gsl::span<const std::byte>& data,
      const WriteModelOptions& options = WriteModelOptions()) const;

  /**
   * @brief Writes an image from a buffer.
   *
   * The [stb_image](https://github.com/nothings/stb) library is used to decode
   * images in `JPG`, `PNG`, `TGA`, `BMP`, `PSD`, `GIF`, `HDR`, or `PIC` format.
   *
   * @param data The buffer from which to write the image.
   * @return The result of writing the image.
   */
  ImageWriterResult writeImage(const gsl::span<const std::byte>& data) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace CesiumGltf
