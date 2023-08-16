#pragma once

#include "Cesium3DTilesReader/Library.h"

#include <Cesium3DTiles/Schema.h>
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
 * @brief The result of reading a schema with
 * {@link SchemaReader::readSchema}.
 */
struct CESIUM3DTILESREADER_API SchemaReaderResult {
  /**
   * @brief The read schema, or std::nullopt if the schema could not be read.
   */
  std::optional<Cesium3DTiles::Schema> schema;

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
 * @brief Reads schemas.
 */
class CESIUM3DTILESREADER_API SchemaReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  SchemaReader();

  /**
   * @brief Gets the options controlling how the JSON is read.
   */
  CesiumJsonReader::JsonReaderOptions& getOptions();

  /**
   * @brief Gets the options controlling how the JSON is read.
   */
  const CesiumJsonReader::JsonReaderOptions& getOptions() const;

  /**
   * @brief Reads a schema.
   *
   * @param data The buffer from which to read the schema.
   * @return The result of reading the schame.
   */
  SchemaReaderResult readSchema(const gsl::span<const std::byte>& data) const;

private:
  CesiumJsonReader::JsonReaderOptions _context;
};

} // namespace Cesium3DTilesReader
