#include "Cesium3DTiles/TileID.h"

#include <string>
#include <variant>

namespace Cesium3DTiles {

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

/*static*/ std::optional<TileID>
TileIdUtilities::getParentTileID(const TileID& tileId) {
  struct Operation {
    std::optional<TileID> operator()(const std::string& /*url*/) {
      return std::nullopt;
    }

    std::optional<TileID>
    operator()(const CesiumGeometry::QuadtreeTileID& quadtreeTileId) {
      if (quadtreeTileId.level == 0) {
        return std::nullopt;
      }

      return CesiumGeometry::QuadtreeTileID(
          quadtreeTileId.level - 1,
          quadtreeTileId.x >> 1,
          quadtreeTileId.y >> 1);
    }

    std::optional<TileID>
    operator()(const CesiumGeometry::OctreeTileID& octreeTileId) {
      if (octreeTileId.level == 0) {
        return std::nullopt;
      }

      return CesiumGeometry::OctreeTileID(
          octreeTileId.level - 1,
          octreeTileId.x >> 1,
          octreeTileId.y >> 1,
          octreeTileId.z >> 1);
    }

    std::optional<TileID> operator()(
        const CesiumGeometry::UpsampledQuadtreeNode& upsampledQuadtreeNode) {
      if (upsampledQuadtreeNode.tileID.level == 0) {
        return std::nullopt;
      }

      return CesiumGeometry::UpsampledQuadtreeNode{
          CesiumGeometry::QuadtreeTileID(
              upsampledQuadtreeNode.tileID.level - 1,
              upsampledQuadtreeNode.tileID.x >> 1,
              upsampledQuadtreeNode.tileID.y >> 1)};
    }
  };

  return std::visit(Operation{}, tileId);
}

} // namespace Cesium3DTiles
