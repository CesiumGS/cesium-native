#pragma once

#include <CesiumGltfWriter/Library.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>

// forward declarations
namespace CesiumGltf {
struct Schema;
}

namespace CesiumGltfWriter {

/**
 * @brief The result of writing a schema with
 * {@link SchemaWriter::writeSchema}.
 */
struct CESIUMGLTFWRITER_API SchemaWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the schema JSON.
   */
  std::vector<std::byte> schemaBytes;

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
 * @brief Options for how to write a schema.
 */
struct CESIUMGLTFWRITER_API SchemaWriterOptions {
  /**
   * @brief If the schema JSON should be pretty printed.
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes schemas.
 */
class CESIUMGLTFWRITER_API SchemaWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  SchemaWriter();

  /**
   * @brief Gets the context used to control how schema extensions are written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how schema extensions are written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Serializes the provided schema object into a byte vector using the
   * provided flags to convert.
   *
   * @param schema The schema.
   * @param options Options for how to write the schema.
   * @return The result of writing the schema.
   */
  SchemaWriterResult writeSchema(
      const CesiumGltf::Schema& schema,
      const SchemaWriterOptions& options = SchemaWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace CesiumGltfWriter
