#pragma once

#include <CesiumUtility/Result.h>
#include <CesiumVectorData/Library.h>
#include <CesiumVectorData/VectorNode.h>

namespace CesiumVectorData {

/**
 * @brief A vector document parsed from a format like GeoJSON.
 *
 * The document is represented as a hierarchy of \ref VectorNode values starting
 * with the root node.
 */
class CESIUMVECTORDATA_API VectorDocument {
public:
  /**
   * @brief Attempts to parse a \ref VectorDocument from the provided GeoJSON.
   *
   * @param bytes The GeoJSON data to parse.
   * @returns A \ref Result containing the parsed \ref VectorDocument or any
   * errors and warnings that came up while parsing.
   */
  static CesiumUtility::Result<VectorDocument>
  fromGeoJson(const std::span<const std::byte>& bytes);

  /**
   * @brief Creates a new \ref VectorDocument directly from a \ref VectorNode.
   */
  VectorDocument(VectorNode&& rootNode);

  /**
   * @brief Obtains the root node of this \ref VectorDocument.
   */
  const VectorNode& getRootNode() const;

private:
  VectorNode _rootNode;
};
} // namespace CesiumVectorData