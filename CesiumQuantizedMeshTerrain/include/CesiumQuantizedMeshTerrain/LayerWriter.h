#pragma once

#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumQuantizedMeshTerrain/Library.h>

#include <span>

// forward declarations
namespace CesiumQuantizedMeshTerrain {
struct Layer;
}

namespace CesiumQuantizedMeshTerrain {

/**
 * @brief The result of writing a layer.json with {@link LayerWriter::write}.
 */
struct CESIUMQUANTIZEDMESHTERRAIN_API LayerWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the layer.json.
   */
  std::vector<std::byte> bytes;

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
 * @brief Options for how to write a layer.json.
 */
struct CESIUMQUANTIZEDMESHTERRAIN_API LayerWriterOptions {
  /**
   * @brief If the layer.json should be pretty printed.
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes layer.json.
 */
class CESIUMQUANTIZEDMESHTERRAIN_API LayerWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  LayerWriter();

  /**
   * @brief Gets the context used to control how layer.json extensions are
   * written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how layer.json extensions are
   * written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Serializes the provided `Layer` into a layer.json byte vector.
   *
   * @param layer The layer.
   * @param options Options for how to write the layer.json.
   * @return The result of writing the layer.json.
   */
  LayerWriterResult write(
      const CesiumQuantizedMeshTerrain::Layer& layer,
      const LayerWriterOptions& options = LayerWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace CesiumQuantizedMeshTerrain
