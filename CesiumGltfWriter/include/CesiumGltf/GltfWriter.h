#pragma once

#include "WriterLibrary.h"

#include <CesiumJsonWriter/ExtensionWriterContext.h>


namespace CesiumGltf {

/**
 * @brief The result of writing a glTF model with
 * {@link GltfWriter::writeModelAsEmbeddedBytes} or {@link GltfWriter::writeModelAndExternalFiles}.
 */
struct CESIUMGLTFWRITER_API ModelWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the glTF asset (as a
   * GLB or glTF).
   */
  std::vector<std::byte> gltfAssetBytes;

  /**
   * @brief Errors, if any, that occurred during the write process.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warnings, if any, that occurred during the write process.
   */
  std::vector<std::string> warnings;
};

enum class GltfExportType { GLB, GLTF };

/**
 * @brief Options for how to write a glTF.
 */
struct CESIUMGLTFWRITER_API WriteModelOptions {
  /**
   * @brief If the glTF asset should be exported as a GLB (binary) or glTF (JSON
   * string)
   */
  GltfExportType exportType = GltfExportType::GLB;

  /**
   * @brief If the glTF asset should be pretty printed. Usable with glTF or GLB
   * (not advised).
   */
  bool prettyPrint = false;

  /**
   * @brief If {@link CesiumGltf::BufferCesium} and
   * {@link CesiumGltf::ImageCesium} should be automatically encoded into base64
   * uris or not.
   */
  bool autoConvertDataToBase64 = false;
};

/**
 * @brief Writes glTF models.
 */
class CESIUMGLTFWRITER_API GltfWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  GltfWriter();

  /**
   * @brief Gets the context used to control how glTF extensions are written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how glTF extensions are written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Writes a glTF or binary glTF (GLB) from a buffer.
   *
   * @param data The buffer from which to write the glTF.
   * @param options Options for how to write the glTF.
   * @return The result of writing the glTF.
   */

  /**
   * @brief Write a glTF or glb asset to a byte vector.
   *
   * @param model Final assembled glTF asset, ready for serialization.
   * @param options Options to use for exporting the asset.
   *
   * @returns A {@link CesiumGltf::ModelWriterResult} containing a list of
   * errors, warnings, and the final generated std::vector<std::byte> of the
   * glTF asset (as a GLB or GLTF).
   *
   * @details Serializes the provided model object into a byte vector using the
   * provided flags to convert. There are a few special scenarios with
   * serializing {@link CesiumGltf::Buffer} and {@link CesiumGltf::Image}
   * objects:
   * - If {@link GltfExportType::GLB} is specified, `model.buffers[0].cesium.data`
   * will be used as the single binary data storage GLB chunk, so it's the
   * callers responsibility to place all their binary data in
   * `model.buffers[0].cesium.data` if they want it to be serialized to the GLB.
   * - If {@link GltfExportType::GLB} is specified, `model.buffers[0].uri` CANNOT
   * be set or a URIErroneouslyDefined error will be returned.
   * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
   * a contains base64 data uri and its `cesium.data` or `cesium.pixelData`
   * vector is non empty, a AmbiguousDataSource error will be returned.
   * - If a {@link CesiumGltf::Buffer} contains a base64 data uri and its
   * byteLength is not set, a ByteLengthNotSet error will be returned.
   * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
   * contains an external file uri and its `cesium.data` or `cesium.pixelData`
   * vector is empty, a MissingDataSource error will be returned.
   * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
   * contains an external file uri, it will be ignored (use
   * {@link CesiumGltf::writeModelAndExternalFiles} for external file
   * support).
   */
  ModelWriterResult writeModelAsEmbeddedBytes(
      const Model& model,
      const WriteModelOptions& options) const;

  /**
   * @brief Write a glTF or glb asset and its associated external images and
   * buffers to multiple files via user provided callback.
   *
   * @returns A {@link CesiumGltf::WriteModelResult} containing a list of
   * errors, warnings, and the final generated std::vector<std::byte> of the
   * glTF asset (as a GLB or GLTF).
   *
   * @param model Final assembled glTF asset, ready for serialization.
   * @param options Options to use for exporting the asset.
   * @param filename Filename of the final glTF / GLB asset or an encountered
   * external {@link CesiumGltf::ImageCesium} / {@link CesiumGltf::BufferCesium}
   * during the serialization process
   * @param WriteGltfCallback Callback the user must implement. The serializer
   * will invoke this callback everytime it encounters
   * a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
   * with an external file uri, giving the callback a chance to write the asset
   * to disk. The callback will be called a final time with the provided
   * filename string when its time to serialize the final `glb` or `gltf` to
   * disk.
   * @details Similar to {@link CesiumGltf::writeModelAsEmbeddedBytes}, with the
   * key variation that a filename will be automatically generated
   * for your {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}  if no
   * uri is provided but `buffer.cesium.data` or `image.cesium.pixelData`
   * is non-empty.
   */

  CESIUMGLTFWRITER_API ModelWriterResult writeModelAndExternalFiles(
      const Model& model,
      const WriteModelOptions& options,
      std::string_view filename,
      const WriteGltfCallback& writeGltfCallback);

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace CesiumGltf
