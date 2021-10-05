#include "Cesium3DTilesSelection/ImplicitTileset.h"

#include "CesiumGeometry/OrientedBoundingBox.h"

using namespace CesiumGeometry;

namespace Cesium3DTilesSelection {

ImplicitTileset::ImplicitTileset(
    const std::string& baseResource,
    const rapidjson::Value& implicitTilingExtensionJson,
    double geometricError,
    const BoundingVolume& boundingVolume,
    TileRefine refine,
    const std::string& contentUriTemplate)
    : _baseResource(baseResource),
      _successfullyParsed(false),
      _geometricError(geometricError),
      // _metadataSchema ?
      _boundingVolume(boundingVolume),
      _refine(refine),
      _subtreeUriTemplate(""),
      _contentUriTemplate(contentUriTemplate),
      _contentHeaders(),
      _tileHeader(""),
      _tilingScheme(ImplicitTilingScheme::Quadtree),
      _branchingFactor(4),
      _subtreeLevels(0),
      _maximumLevel(0) {

  if (!implicitTilingExtensionJson.IsObject()) {
    return;
  }

  auto extension = implicitTilingExtensionJson.GetObject();

  auto tilingSchemeIt = extension.FindMember("subdivisionScheme");
  auto subtreeLevelsIt = extension.FindMember("subtreeLevels");
  auto maximumLevelIt = extension.FindMember("maximumLevel");
  auto subtreesIt = extension.FindMember("subtrees");

  if (tilingSchemeIt == extension.MemberEnd() ||
      !tilingSchemeIt->value.IsString() ||
      subtreeLevelsIt == extension.MemberEnd() ||
      !subtreeLevelsIt->value.IsInt() ||
      maximumLevelIt == extension.MemberEnd() ||
      !maximumLevelIt->value.IsInt() || subtreesIt == extension.MemberEnd() ||
      !subtreesIt->value.IsObject()) {
    return;
  }

  auto subtrees = subtreesIt->value.GetObject();
  auto subtreesUriIt = subtrees.FindMember("uri");

  if (subtreesUriIt == subtrees.MemberEnd() ||
      !subtreesUriIt->value.IsString()) {
    return;
  }

  const char* tilingScheme = tilingSchemeIt->value.GetString();
  if (std::strcmp(tilingScheme, "QUADTREE")) {
    this->_tilingScheme = ImplicitTilingScheme::Quadtree;
    this->_branchingFactor = 4;
  } else if (std::strcmp(tilingScheme, "OCTREE")) {
    this->_tilingScheme = ImplicitTilingScheme::Octree;
    this->_branchingFactor = 8;
  } else {
    return;
  }

  this->_subtreeLevels = subtreeLevelsIt->value.GetInt();
  this->_maximumLevel = maximumLevelIt->value.GetInt();
  this->_subtreeUriTemplate = subtreesUriIt->value.GetString();
}

} // namespace Cesium3DTilesSelection