#pragma once

#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumTilesetJsonWriter/Library.h>

// forward declarations
namespace CesiumTilesetJson {
struct ConditionalContent;
}

namespace CesiumTilesetJsonWriter {

/**
 * @brief The result of writing a @ref CesiumTilesetJson::ConditionalContent
 * with
 * @ref ConditionalContentWriter::writeConditionalContent.
 */
struct CESIUMTILESETJSONWRITER_API ConditionalContentWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the conditional
   * content JSON.
   */
  std::vector<std::byte> conditionalContentBytes;

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
 * @brief Options for how to write a @ref CesiumTilesetJson::ConditionalContent.
 */
struct CESIUMTILESETJSONWRITER_API ConditionalContentWriterOptions {
  /**
   * @brief If the conditional content JSON should be pretty printed.
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes @ref CesiumTilesetJson::ConditionalContent.
 */
class CESIUMTILESETJSONWRITER_API ConditionalContentWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  ConditionalContentWriter();

  /**
   * @brief Gets the context used to control how conditional content extensions
   * are written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how conditional content extensions
   * are written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Serializes the provided conditional content object into a byte
   * vector using the provided flags to convert.
   *
   * @param conditionalContent The conditional content.
   * @param options Options for how to write the conditional content.
   * @return The result of writing the conditional content.
   */
  ConditionalContentWriterResult writeConditionalContent(
      const CesiumTilesetJson::ConditionalContent& conditionalContent,
      const ConditionalContentWriterOptions& options =
          ConditionalContentWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace CesiumTilesetJsonWriter
