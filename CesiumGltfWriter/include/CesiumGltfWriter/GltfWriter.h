#pragma once

#include <CesiumGltfWriter/Library.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>

#include <span>

// forward declarations
namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfWriter {

/**
 * @brief The result of writing a glTF with
 * {@link GltfWriter::writeGltf} or {@link GltfWriter::writeGlb}
 */
struct CESIUMGLTFWRITER_API GltfWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the glTF or glb.
   */
  std::vector<std::byte> gltfBytes;

  /**
   * @brief Errors, if any, that occurred during the write process.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warnings, if any, that occurred during the write process.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief Options for how to write a glTF.
 */
struct CESIUMGLTFWRITER_API GltfWriterOptions {
  /**
   * @brief If the glTF JSON should be pretty printed. Usable with glTF or GLB
   * (not advised).
   */
  bool prettyPrint = false;

  /**
   * @brief Byte alignment of the GLB binary chunk. When using 64-bit types in
   * EXT_mesh_features this value should be set to 8.
   */
  size_t binaryChunkByteAlignment = 4;
};

/**
 * @brief Writes glTF.
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
   * @brief Serializes the provided model into a glTF JSON byte vector.
   *
   * @details Ignores internal data such as {@link CesiumGltf::BufferCesium}
   * and {@link CesiumGltf::ImageAsset} when serializing the glTF. Internal
   * data must either be converted to data uris or saved as external files. The
   * buffer.uri and image.uri fields must be set accordingly prior to calling
   * this function.
   *
   * @param model The model.
   * @param options Options for how to write the glTF.
   * @return The result of writing the glTF.
   */
  GltfWriterResult writeGltf(
      const CesiumGltf::Model& model,
      const GltfWriterOptions& options = GltfWriterOptions()) const;

  /**
   * @brief Serializes the provided model into a glb byte vector.
   *
   * @details The first buffer object implicitly refers to the GLB binary chunk
   * and should not have a uri. Ignores internal data such as
   * {@link CesiumGltf::BufferCesium} and {@link CesiumGltf::ImageAsset}.
   *
   * @param model The model.
   * @param bufferData The buffer data to store in the GLB binary chunk.
   * @param options Options for how to write the glb.
   * @return The result of writing the glb.
   */
  GltfWriterResult writeGlb(
      const CesiumGltf::Model& model,
      const std::span<const std::byte>& bufferData,
      const GltfWriterOptions& options = GltfWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace CesiumGltfWriter
