#include <Cesium3DTilesContent/ImplicitTiling.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/Uri.h>

namespace Cesium3DTilesContent {

/*static*/ std::string ImplicitTiling::resolveUrl(
    const std::string& baseUrl,
    const std::string& urlTemplate,
    const CesiumGeometry::QuadtreeTileID& quadtreeID) {
  std::string url = CesiumUtility::Uri::substituteTemplateParameters(
      urlTemplate,
      [&quadtreeID](const std::string& placeholder) {
        if (placeholder == "level") {
          return std::to_string(quadtreeID.level);
        }
        if (placeholder == "x") {
          return std::to_string(quadtreeID.x);
        }
        if (placeholder == "y") {
          return std::to_string(quadtreeID.y);
        }

        return placeholder;
      });

  return CesiumUtility::Uri::resolve(baseUrl, url);
}

/*static*/ std::string ImplicitTiling::resolveUrl(
    const std::string& baseUrl,
    const std::string& urlTemplate,
    const CesiumGeometry::OctreeTileID& octreeID) {
  std::string url = CesiumUtility::Uri::substituteTemplateParameters(
      urlTemplate,
      [&octreeID](const std::string& placeholder) {
        if (placeholder == "level") {
          return std::to_string(octreeID.level);
        }
        if (placeholder == "x") {
          return std::to_string(octreeID.x);
        }
        if (placeholder == "y") {
          return std::to_string(octreeID.y);
        }
        if (placeholder == "z") {
          return std::to_string(octreeID.z);
        }

        return placeholder;
      });

  return CesiumUtility::Uri::resolve(baseUrl, url);
}

} // namespace Cesium3DTilesContent
