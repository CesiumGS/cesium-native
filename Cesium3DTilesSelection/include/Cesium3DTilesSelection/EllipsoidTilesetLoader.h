#pragma once

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGltf/Model.h>

#include <glm/fwd.hpp>

#include <cstdint>
#include <memory>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumAsync;
using namespace CesiumUtility;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {
class EllipsoidTilesetLoader : public TilesetContentLoader {
public:
  EllipsoidTilesetLoader(const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

  static std::unique_ptr<Tileset> createTileset(
      const TilesetExternals& externals,
      const Ellipsoid& ellipsoid = Ellipsoid::WGS84,
      const TilesetOptions& options = TilesetOptions{});

  Future<TileLoadResult> loadTileContent(const TileLoadInput& input) override;
  TileChildrenResult createTileChildren(const Tile& tile) override;

private:
  struct Geometry {
    std::vector<uint16_t> indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
  };

  BoundingRegion createBoundingRegion(const QuadtreeTileID& quadtreeID) const;
  Geometry createGeometry(const Tile& tile) const;
  Model createModel(const Geometry& geometry) const;

  GeographicProjection _projection;
  QuadtreeTilingScheme _tilingScheme;
};
} // namespace Cesium3DTilesSelection
