#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/TileRefine.h"
#include "rapidjson/document.h"

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

enum class ImplicitTilingScheme { Quadtree = 0, Octree = 1 };

class ImplicitTileset {
public:
  ImplicitTileset(
      const std::string& baseResource,
      const rapidjson::Value& implicitTilingExtensionJson,
      double geometricError,
      const BoundingVolume& boundingVolume,
      TileRefine refine,
      const std::string& contentUriTemplate);

  bool isSuccessfullyParsed() const { return _successfullyParsed; }

private:
  std::string _baseResource;
  bool _successfullyParsed;
  double _geometricError;
  // _metadataScema ?
  BoundingVolume _boundingVolume;
  TileRefine _refine;
  std::string _subtreeUriTemplate;
  std::string _contentUriTemplate;
  std::vector<std::string> _contentHeaders;
  std::string _tileHeader;
  ImplicitTilingScheme _tilingScheme;
  int8_t _branchingFactor;
  int32_t _subtreeLevels;
  int32_t _maximumLevel;
};

} // namespace Cesium3DTilesSelection