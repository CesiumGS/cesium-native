#pragma once

#include <CesiumUtility/Result.h>
#include <CesiumVectorData/Library.h>
#include <CesiumVectorData/VectorNode.h>

namespace CesiumVectorData {
class CESIUMVECTORDATA_API VectorDocument {
public:
  static CesiumUtility::Result<VectorDocument>
  fromGeoJson(const std::span<std::byte>& bytes);

  VectorDocument(VectorNode&& rootNode) : _rootNode(std::move(rootNode)) {}

private:
  VectorNode _rootNode;
};
} // namespace CesiumVectorData