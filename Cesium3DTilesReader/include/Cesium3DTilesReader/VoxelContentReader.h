#pragma once

#include "Cesium3DTilesReader/Library.h"

#include <Cesium3DTiles/VoxelContent.h>
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
 * @brief The result of reading voxel content with
 * {@link VoxelContentReader::readVoxelContent}.
 */
struct CESIUM3DTILESREADER_API VoxelContentReaderResult {
  /**
   * @brief The read voxel content, or std::nullopt if the voxel content could
   * not be read.
   */
  std::optional<Cesium3DTiles::VoxelContent> voxels;

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
 * @brief Reads voxel content.
 */
class CESIUM3DTILESREADER_API VoxelContentReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  VoxelContentReader();

  /**
   * @brief Gets the context used to control how extensions are loaded from
   * voxel content.
   */
  CesiumJsonReader::ExtensionReaderContext& getExtensions();

  /**
   * @brief Gets the context used to control how extensions are loaded from
   * voxel content.
   */
  const CesiumJsonReader::ExtensionReaderContext& getExtensions() const;

  /**
   * @brief Reads voxel content.
   *
   * @param data The buffer from which to read the voxel content.
   * @param options Options for how to read the voxel content.
   * @return The result of reading the voxel content.
   */
  VoxelContentReaderResult
  readVoxelContent(const gsl::span<const std::byte>& data) const;

private:
  CesiumJsonReader::ExtensionReaderContext _context;
};

} // namespace Cesium3DTilesReader
