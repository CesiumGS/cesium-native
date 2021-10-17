#pragma once

#include "WriterLibrary.h"

#include <Cesium3DTiles/Tileset.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>

namespace Cesium3DTiles {

/**
 * @brief The result of writing a tileset with
 * {@link Cesium3DTilesWriter::writeTileset}.
 */
struct CESIUM3DTILESWRITER_API TilesetWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the tileset.
   */
  std::vector<std::byte> tilesetBytes;

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
 * @brief Options for how to write a tileset.
 */
struct CESIUM3DTILESWRITER_API WriteTilesetOptions {
  /**
   * @brief If the tileset JSON should be pretty printed.
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes tilesets.
 */
class CESIUM3DTILESWRITER_API Cesium3DTilesWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  Cesium3DTilesWriter();

  /**
   * @brief Gets the context used to control how tileset extensions are written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how tileset extensions are written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Serializes the provided tileset object into a byte vector using the
   * provided flags to convert.
   *
   * @param data The buffer from which to write the tileset.
   * @param options Options for how to write the tileset.
   * @return The result of writing the tileset.
   */
  TilesetWriterResult writeTileset(
      const Tileset& tileset,
      const WriteTilesetOptions& options = WriteTilesetOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace Cesium3DTiles
