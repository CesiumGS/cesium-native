#pragma once

#include "WriterLibrary.h"

#include <Cesium3DTiles/Tileset.h>
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

namespace Cesium3DTiles {

/**
 * @brief The result of writing a tileset with
 * {@link TilesetWriter::writeTileset}.
 */
struct CESIUM3DTILESWRITER_API TilesetWriterResult {
  /**
   * @brief The write tileset, or std::nullopt if the tileset could not be write.
   */
  std::optional<Tileset> tileset;

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
struct CESIUM3DTILESWRITER_API WriteTilesetOptions {};

/**
 * @brief Writes tilesets.
 */
class CESIUM3DTILESWRITER_API TilesetWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  TilesetWriter();

  /**
   * @brief Gets the context used to control how extensions are written from a
   * tileset.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how extensions are written from a
   * tileset.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Writes a tileset.
   *
   * @param data The buffer from which to write the tileset.
   * @param options Options for how to write the tileset.
   * @return The result of writing the tileset.
   */
  TilesetWriterResult writeTileset(
      const gsl::span<const std::byte>& data,
      const WriteTilesetOptions& options = WriteTilesetOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace Cesium3DTiles
