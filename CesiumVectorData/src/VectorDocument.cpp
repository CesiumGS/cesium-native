#include <CesiumVectorData/VectorDocument.h>

#include "GeoJsonParser.h"

using namespace CesiumUtility;

namespace CesiumVectorData {

Result<VectorDocument> VectorDocument::fromGeoJson(const std::span<std::byte> &bytes) {
  Result<VectorNode> parseResult = parseGeoJson(bytes);
  if(!parseResult.value) {
    return Result<VectorDocument>(std::move(parseResult.errors));
  }

  return Result<VectorDocument>(VectorDocument(std::move(*parseResult.value)));
}

}