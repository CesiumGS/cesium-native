#pragma once

#include <Cesium3DTilesWriter/Library.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>

// forward declarations
namespace Cesium3DTiles {
struct ConditionalContent;
}

namespace Cesium3DTilesWriter {

/**
 * @brief The result of writing a @ref Cesium3DTiles::ConditionalContent with
 * @ref ConditionalContentWriter::writeConditionalContent.
 */
struct CESIUM3DTILESWRITER_API ConditionalContentWriterResult {
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
 * @brief Options for how to write a @ref Cesium3DTiles::ConditionalContent.
 */
struct CESIUM3DTILESWRITER_API ConditionalContentWriterOptions {
  /**
   * @brief If the conditional content JSON should be pretty printed.
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes @ref Cesium3DTiles::ConditionalContent.
 */
class CESIUM3DTILESWRITER_API ConditionalContentWriter {
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
      const Cesium3DTiles::ConditionalContent& conditionalContent,
      const ConditionalContentWriterOptions& options =
          ConditionalContentWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace Cesium3DTilesWriter
