#include "NodeJsonHandlers.h"
#include "PointCloudJsonHandlers.h"

#include <CesiumI3SReader/NodePageReader.h>
#include <CesiumJsonReader/JsonReader.h>

namespace CesiumI3SReader {

NodePageReader::NodePageReader() noexcept = default;

CesiumJsonReader::ReadJsonResult<CesiumI3S::NodePage>
NodePageReader::readNodePage(const std::span<const std::byte>& data) const {
  NodePageJsonHandler handler;
  return CesiumJsonReader::JsonReader::readJson(data, handler);
}

CesiumJsonReader::ReadJsonResult<CesiumI3S::PointCloudNodePage>
NodePageReader::readPointCloudNodePage(
    const std::span<const std::byte>& data) const {
  PointCloudNodePageJsonHandler handler;
  return CesiumJsonReader::JsonReader::readJson(data, handler);
}

} // namespace CesiumI3SReader
