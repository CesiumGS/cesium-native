#pragma once

#include "Cesium3DTilesReader/Library.h"

#include <Cesium3DTiles/Extension3dTilesMetadataSchemaLegacy.h>
#include <CesiumJsonReader/ExtensionReaderContext.h> 

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
 * {@link SchemaReaderLegacy::readSchema}.
 */
struct CESIUM3DTILESREADER_API SchemaReaderResultLegacy {
  /**
   * @brief The read schema, or std::nullopt if the schema could not be read.
   */
  std::optional<Cesium3DTiles::Extension3dTilesMetadataSchemaLegacy> schema;

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
class CESIUM3DTILESREADER_API SchemaReaderLegacy {
public:
  /**
   * @brief Constructs a new instance.
   */
  SchemaReaderLegacy();

  /**
   * @brief Gets the context used to control how extensions are loaded from a
   * schema.
   */
  CesiumJsonReader::ExtensionReaderContext& getExtensions();

  /**
   * @brief Gets the context used to control how extensions are loaded from a
   * schema.
   */
  const CesiumJsonReader::ExtensionReaderContext& getExtensions() const;

  /**
   * @brief Reads a schema.
   *
   * @param data The buffer from which to read the schema.
   * @return The result of reading the schame.
   */
  SchemaReaderResultLegacy readSchema(const gsl::span<const std::byte>& data) const;

private:
  CesiumJsonReader::ExtensionReaderContext _context;
};

} // namespace Cesium3DTilesReader
