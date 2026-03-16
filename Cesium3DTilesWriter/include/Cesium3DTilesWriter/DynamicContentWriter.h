#pragma once

#include <Cesium3DTilesWriter/Library.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>

// forward declarations
namespace Cesium3DTiles {
struct DynamicContent;
}

namespace Cesium3DTilesWriter {

/**
 * @brief The result of writing a dynamicContent with
 * {@link DynamicContentWriter::writeDynamicContent}.
 */
struct CESIUM3DTILESWRITER_API DynamicContentWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the dynamicContent
   * JSON.
   */
  std::vector<std::byte> dynamicContentBytes;

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
 * @brief Options for how to write a dynamicContent.
 */
struct CESIUM3DTILESWRITER_API DynamicContentWriterOptions {
  /**
   * @brief If the dynamicContent JSON should be pretty printed.
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes dynamicContents.
 */
class CESIUM3DTILESWRITER_API DynamicContentWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  DynamicContentWriter();

  /**
   * @brief Gets the context used to control how dynamicContent extensions are
   * written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how dynamicContent extensions are
   * written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Serializes the provided dynamicContent object into a byte vector
   * using the provided flags to convert.
   *
   * @param dynamicContent The dynamicContent.
   * @param options Options for how to write the dynamicContent.
   * @return The result of writing the dynamicContent.
   */
  DynamicContentWriterResult writeDynamicContent(
      const Cesium3DTiles::DynamicContent& dynamicContent,
      const DynamicContentWriterOptions& options =
          DynamicContentWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace Cesium3DTilesWriter
