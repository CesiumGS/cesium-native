#pragma once

#include <CesiumUtility/Result.h>
#include <CesiumVectorData/Library.h>
#include <CesiumVectorData/VectorNode.h>

namespace CesiumVectorData {
class CESIUMVECTORDATA_API VectorDocument {
public:
  static CesiumUtility::Result<VectorDocument>
  fromGeoJson(const std::span<const std::byte>& bytes);

  VectorDocument(VectorNode&& rootNode);

  const VectorNode& getRootNode() const;

private:
  VectorNode _rootNode;
};
} // namespace CesiumVectorData