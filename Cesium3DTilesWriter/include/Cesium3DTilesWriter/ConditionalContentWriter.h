#pragma once

#include <Cesium3DTilesWriter/Library.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>

// forward declarations
namespace Cesium3DTiles {
struct ConditionalContent;
}

namespace Cesium3DTilesWriter {

/**
 * @brief The result of writing a ConditionalContent with
 * {@link ConditionalContentWriter::writeConditionalContent}.
 */
struct CESIUM3DTILESWRITER_API ConditionalContentWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the ConditionalContent
   * JSON.
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
 * @brief Options for how to write a ConditionalContent.
 */
struct CESIUM3DTILESWRITER_API ConditionalContentWriterOptions {
  /**
   * @brief If the ConditionalContent JSON should be pretty printed.
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes ConditionalContents.
 */
class CESIUM3DTILESWRITER_API ConditionalContentWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  ConditionalContentWriter();

  /**
   * @brief Gets the context used to control how ConditionalContent extensions
   * are written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how ConditionalContent extensions
   * are written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Serializes the provided ConditionalContent object into a byte vector
   * using the provided flags to convert.
   *
   * @param conditionalContent The ConditionalContent.
   * @param options Options for how to write the ConditionalContent.
   * @return The result of writing the ConditionalContent.
   */
  ConditionalContentWriterResult writeConditionalContent(
      const Cesium3DTiles::ConditionalContent& conditionalContent,
      const ConditionalContentWriterOptions& options =
          ConditionalContentWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace Cesium3DTilesWriter
