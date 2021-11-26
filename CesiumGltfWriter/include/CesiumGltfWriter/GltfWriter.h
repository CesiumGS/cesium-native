#pragma once

#include "CesiumGltfWriter/Library.h"

#include <CesiumJsonWriter/ExtensionWriterContext.h>

// forward declarations
namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfWriter {

/**
 * @brief The result of writing a glTF with
 * {@link GltfWriter::writeGltf}.
 */
struct CESIUMGLTFWRITER_API GltfWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the glTF.
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

enum class GltfExportType { GLB, GLTF };

/**
 * @brief Options for how to write a glTF.
 */

struct CESIUMGLTFWRITER_API GltfWriterOptions {
  /**
   * @brief If the glTF should be exported as a GLB (binary) or glTF (JSON)
   */
  GltfExportType exportType = GltfExportType::GLB;

  /**
   * @brief If the glTF should be pretty printed. Usable with glTF or GLB (not
   * advised)
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
   * @brief Serializes the provided model into a glTF byte vector using the
   * provided flags to convert.
   *
   * @param model The model.
   * @param options Options for how to write the glTF.
   * @return The result of writing the glTF.
   */
  GltfWriterResult writeGltf(
      const CesiumGltf::Model& model,
      const GltfWriterOptions& options = GltfWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace CesiumGltfWriter
