#include <Cesium3DTilesSelection/TileID.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>

#include <string>
#include <variant>

namespace Cesium3DTilesSelection {

/*static*/ std::string
TileIdUtilities::createTileIdString(const TileID& tileId) {

  struct Operation {

    std::string operator()(const std::string& url) { return url; }

    std::string
    operator()(const CesiumGeometry::QuadtreeTileID& quadtreeTileId) {
      // Strings of the form "L10-X23-Y144"
      return "L" + std::to_string(quadtreeTileId.level) + "-" + "X" +
             std::to_string(quadtreeTileId.x) + "-" + "Y" +
             std::to_string(quadtreeTileId.y);
    }

    std::string operator()(const CesiumGeometry::OctreeTileID& octreeTileId) {
      // Strings of the form "L10-X23-Y144-Z42"
      return "L" + std::to_string(octreeTileId.level) + "-" + "X" +
             std::to_string(octreeTileId.x) + "-" + "Y" +
             std::to_string(octreeTileId.y) + "-" + "Z" +
             std::to_string(octreeTileId.z);
    }

    std::string operator()(
        const CesiumGeometry::UpsampledQuadtreeNode& upsampledQuadtreeNode) {
      // Strings of the form "upsampled-L10-X23-Y144"
      return "upsampled-L" +
             std::to_string(upsampledQuadtreeNode.tileID.level) + "-" + "X" +
             std::to_string(upsampledQuadtreeNode.tileID.x) + "-" + "Y" +
             std::to_string(upsampledQuadtreeNode.tileID.y);
    }
  };
  return std::visit(Operation{}, tileId);
}

} // namespace Cesium3DTilesSelection
