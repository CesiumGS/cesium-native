#pragma once

#include <CesiumI3S/NodeIndexDocument.h>
#include <CesiumI3SReader/Library.h>
#include <CesiumJsonReader/JsonReader.h>

#include <cstddef>
#include <span>

namespace CesiumI3SReader {

/**
 * @brief Reads a legacy I3S node index document from JSON bytes into a
 * strongly-typed i3s spec.
 *
 * @remarks Node index documents are the per-node metadata format used in I3S
 * versions prior to 1.7. Prefer @ref NodePageReader for newer data.
 */
class CESIUMI3SREADER_API NodeIndexDocumentReader {
public:
  /** @brief Constructs a new NodeIndexDocumentReader. */
  NodeIndexDocumentReader() noexcept;

  /**
   * @brief Reads a node index document.
   *
   * @param data The raw JSON bytes of the node index document.
   * @returns The parsed @ref CesiumI3S::NodeIndexDocument, along with any
   * errors or warnings encountered during parsing.
   */
  CesiumJsonReader::ReadJsonResult<CesiumI3S::NodeIndexDocument>
  readNodeIndexDocument(const std::span<const std::byte>& data) const;
};

} // namespace CesiumI3SReader
