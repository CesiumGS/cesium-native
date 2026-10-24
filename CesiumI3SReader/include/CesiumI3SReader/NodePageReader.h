#pragma once

#include <CesiumI3S/Node.h>
#include <CesiumI3S/PointCloud.h>
#include <CesiumI3SReader/Library.h>
#include <CesiumJsonReader/JsonReader.h>

#include <cstddef>
#include <span>

namespace CesiumI3SReader {

/**
 * @brief Reads an I3S node page from JSON bytes into a strongly-typed i3s spec.
 */
class CESIUMI3SREADER_API NodePageReader {
public:
  /** @brief Constructs a new NodePageReader. */
  NodePageReader() noexcept;

  /**
   * @brief Reads a CMN node page.
   *
   * @param data The raw JSON bytes of the node page document.
   * @returns The parsed @ref CesiumI3S::NodePage, along with any errors or
   * warnings encountered during parsing.
   */
  CesiumJsonReader::ReadJsonResult<CesiumI3S::NodePage>
  readNodePage(const std::span<const std::byte>& data) const;

  /**
   * @brief Reads a PCSL point cloud node page.
   *
   * @param data The raw JSON bytes of the node page document.
   * @returns The parsed @ref CesiumI3S::PointCloudNodePage, along with any
   * errors or warnings encountered during parsing.
   */
  CesiumJsonReader::ReadJsonResult<CesiumI3S::PointCloudNodePage>
  readPointCloudNodePage(const std::span<const std::byte>& data) const;
};

} // namespace CesiumI3SReader
