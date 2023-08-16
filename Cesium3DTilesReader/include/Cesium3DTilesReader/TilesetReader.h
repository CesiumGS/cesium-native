#pragma once

#include "Cesium3DTilesReader/Library.h"

#include <Cesium3DTiles/Tileset.h>
#include <CesiumJsonReader/JsonReaderOptions.h>

#include <gsl/span>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Cesium3DTilesReader {

/**
 * @brief The result of reading a tileset with
 * {@link TilesetReader::readTileset}.
 */
struct CESIUM3DTILESREADER_API TilesetReaderResult {
  /**
   * @brief The read tileset, or std::nullopt if the tileset could not be read.
   */
  std::optional<Cesium3DTiles::Tileset> tileset;

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
 * @brief Reads tilesets.
 */
class CESIUM3DTILESREADER_API TilesetReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  TilesetReader();

  /**
   * @brief Gets the options controlling how the JSON is read.
   */
  CesiumJsonReader::JsonReaderOptions& getOptions();

  /**
   * @brief Gets the options controlling how the JSON is read.
   */
  const CesiumJsonReader::JsonReaderOptions& getOptions() const;

  /**
   * @brief Reads a tileset.
   *
   * @param data The buffer from which to read the tileset.
   * @return The result of reading the tileset.
   */
  TilesetReaderResult readTileset(const gsl::span<const std::byte>& data) const;

private:
  CesiumJsonReader::JsonReaderOptions _context;
};

} // namespace Cesium3DTilesReader
