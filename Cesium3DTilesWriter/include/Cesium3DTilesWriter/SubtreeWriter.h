#pragma once

#include <Cesium3DTilesWriter/Library.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>

#include <span>

// forward declarations
namespace Cesium3DTiles {
struct Subtree;
}

namespace Cesium3DTilesWriter {

/**
 * @brief The result of writing a subtree with
 * {@link SubtreeWriter::writeSubtreeJson} or {@link SubtreeWriter::writeSubtreeBinary}.
 */
struct CESIUM3DTILESWRITER_API SubtreeWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the subtree JSON or
   * subtree binary.
   */
  std::vector<std::byte> subtreeBytes;

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
 * @brief Options for how to write a subtree.
 */
struct CESIUM3DTILESWRITER_API SubtreeWriterOptions {
  /**
   * @brief If the subtree JSON should be pretty printed. Usable with subtree
   * JSON or subtree binary (not advised).
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes subtrees.
 */
class CESIUM3DTILESWRITER_API SubtreeWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  SubtreeWriter();

  /**
   * @brief Gets the context used to control how subtree extensions are written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how subtree extensions are written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Serializes the provided subtree into a subtree JSON byte vector.
   *
   * Ignores internal data such as {@link Cesium3DTiles::BufferCesium} when
   * serializing the subtree. Internal data must be saved as external files. The
   * buffer.uri field must be set accordingly prior to calling this function.
   *
   * @param subtree The subtree.
   * @param options Options for how to write the subtree.
   * @return The result of writing the subtree.
   */
  SubtreeWriterResult writeSubtreeJson(
      const Cesium3DTiles::Subtree& subtree,
      const SubtreeWriterOptions& options = SubtreeWriterOptions()) const;

  /**
   * @brief Serializes the provided subtree into a subtree binary byte vector.
   *
   * The first buffer object implicitly refers to the subtree binary section
   * and should not have a uri. Ignores internal data such as
   * {@link Cesium3DTiles::BufferCesium}.
   *
   * @param subtree The subtree.
   * @param bufferData The buffer data to store in the subtree binary section.
   * @param options Options for how to write the subtree.
   * @return The result of writing the subtree.
   */
  SubtreeWriterResult writeSubtreeBinary(
      const Cesium3DTiles::Subtree& subtree,
      const std::span<const std::byte>& bufferData,
      const SubtreeWriterOptions& options = SubtreeWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace Cesium3DTilesWriter
