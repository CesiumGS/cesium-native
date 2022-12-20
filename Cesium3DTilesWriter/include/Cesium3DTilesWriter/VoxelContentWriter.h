#pragma once

#include "Cesium3DTilesWriter/Library.h"

#include <CesiumJsonWriter/ExtensionWriterContext.h>

// forward declarations
namespace Cesium3DTiles {
struct VoxelContent;
}

namespace Cesium3DTilesWriter {

/**
 * @brief The result of writing voxel content with
 * {@link VoxelContentWriterWriter::writeVoxelContent}.
 */
struct CESIUM3DTILESWRITER_API VoxelContentWriterResult {
  /**
   * @brief The final generated std::vector<std::byte> of the voxel JSON.
   */
  std::vector<std::byte> voxelContentBytes;

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
 * @brief Options for how to write voxel content.
 */
struct CESIUM3DTILESWRITER_API VoxelContentWriterOptions {
  /**
   * @brief If the voxel JSON should be pretty printed.
   */
  bool prettyPrint = false;
};

/**
 * @brief Writes voxel content.
 */
class CESIUM3DTILESWRITER_API VoxelContentWriter {
public:
  /**
   * @brief Constructs a new instance.
   */
  VoxelContentWriter();

  /**
   * @brief Gets the context used to control how voxel content extensions are
   * written.
   */
  CesiumJsonWriter::ExtensionWriterContext& getExtensions();

  /**
   * @brief Gets the context used to control how voxel content extensions are
   * written.
   */
  const CesiumJsonWriter::ExtensionWriterContext& getExtensions() const;

  /**
   * @brief Serializes the provided voxel content object into a byte vector
   * using the provided flags to convert.
   *
   * @param voxel The voxel.
   * @param options Options for how to write the voxel.
   * @return The result of writing the voxel.
   */
  VoxelContentWriterResult writeVoxelContent(
      const Cesium3DTiles::VoxelContent& voxel,
      const VoxelContentWriterOptions& options =
          VoxelContentWriterOptions()) const;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace Cesium3DTilesWriter
