#include "GeoJsonParser.h"

#include <CesiumUtility/Result.h>
#include <CesiumVectorData/VectorDocument.h>

#include <cstddef>
#include <span>
#include <utility>

using namespace CesiumUtility;

namespace CesiumVectorData {

Result<VectorDocument>
VectorDocument::fromGeoJson(const std::span<const std::byte>& bytes) {
  Result<VectorNode> parseResult = parseGeoJson(bytes);
  if (!parseResult.value) {
    return Result<VectorDocument>(std::move(parseResult.errors));
  }

  return Result<VectorDocument>(
      VectorDocument(std::move(*parseResult.value)),
      std::move(parseResult.errors));
}

VectorDocument::VectorDocument(VectorNode&& rootNode)
    : _rootNode(std::move(rootNode)) {}

const VectorNode& VectorDocument::getRootNode() const { return _rootNode; }
} // namespace CesiumVectorData