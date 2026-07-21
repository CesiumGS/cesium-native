#include "NodeIndexDocumentJsonHandler.h"

#include <CesiumI3SReader/NodeIndexDocumentReader.h>
#include <CesiumJsonReader/JsonReader.h>

namespace CesiumI3SReader {

NodeIndexDocumentReader::NodeIndexDocumentReader() noexcept = default;

CesiumJsonReader::ReadJsonResult<CesiumI3S::NodeIndexDocument>
NodeIndexDocumentReader::readNodeIndexDocument(
    const std::span<const std::byte>& data) const {
  NodeIndexDocumentJsonHandler handler;
  return CesiumJsonReader::JsonReader::readJson(data, handler);
}

} // namespace CesiumI3SReader
