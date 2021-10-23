#pragma once

#include "ReaderLibrary.h"

#include <Cesium3DTiles/Tileset.h>
#include <CesiumJsonReader/ExtensionReaderContext.h>
#include <CesiumJsonReader/IExtensionJsonHandler.h>

#include <gsl/span>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTiles {

/**
 * @brief The result of reading a tileset with
 * {@link TilesetReader::readTileset}.
 */
struct CESIUM3DTILESREADER_API TilesetReaderResult {
  /**
   * @brief The read tileset, or std::nullopt if the tileset could not be read.
   */
  std::optional<Tileset> tileset;

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
 * @brief Options for how to read a tileset.
 */
struct CESIUM3DTILESREADER_API ReadTilesetOptions {};

/**
 * @brief Reads tilesets.
 */
class CESIUM3DTILESREADER_API TilesetReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  TilesetReader();

  /**
   * @brief Gets the context used to control how extensions are loaded from a
   * tileset.
   */
  CesiumJsonReader::ExtensionReaderContext& getExtensions();

  /**
   * @brief Gets the context used to control how extensions are loaded from a
   * tileset.
   */
  const CesiumJsonReader::ExtensionReaderContext& getExtensions() const;

  /**
   * @brief Reads a tileset.
   *
   * @param data The buffer from which to read the tileset.
   * @param options Options for how to read the tileset.
   * @return The result of reading the tileset.
   */
  TilesetReaderResult readTileset(
      const gsl::span<const std::byte>& data,
      const ReadTilesetOptions& options = ReadTilesetOptions()) const;

private:
  CesiumJsonReader::ExtensionReaderContext _context;
};

} // namespace Cesium3DTiles
